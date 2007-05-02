#include <stdlib.h>
#include <string.h>

#include "context.h"
#include "filedata.h"
#include "list.h"
#include "logging.h"

static LIST_HEAD(context_list);

struct context * create_context(const char *filename)
{
	struct context *ctx = malloc(sizeof(struct context));
	if (ctx == NULL) {
		log_print(LOG_WARN, "create_context(): out of memory");
		return NULL;
	}

	memset(ctx, 0, sizeof(struct context));

	ctx->lbuf = create_linebuffer(256);
	if (ctx->lbuf == NULL) {
		free(ctx);
		return NULL;
	}

	ctx->file = get_filedata(filename);
	if (ctx->file == NULL) {
		free(ctx->lbuf);
		free(ctx);
		return NULL;
	}

	list_add(&ctx->list, &context_list);
	return ctx;
}

int destroy_context(struct context *ctx)
{
	list_del(&ctx->list);

	if (ctx->dev_close != NULL)
		ctx->dev_close(ctx);

	free(ctx->file);
	free(ctx->lbuf);
	free(ctx);
	return 0;
}

int context_close(void)
{
	struct context *ctx, *tmp;

	list_for_each_entry_safe(ctx, tmp, &context_list, list)
		destroy_context(ctx);

	return 0;
}
