// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 MediaTek Inc.
 *
 * Author: Chris.Chou <chris.chou@mediatek.com>
 *         Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include <linux/bitops.h>
#include <crypto/aes.h>
#include <crypto/internal/skcipher.h>

#include "crypto-eip/crypto-eip.h"
#include "crypto-eip/ddk-wrapper.h"
#include "crypto-eip/lookaside.h"
#include "crypto-eip/internal.h"

void mtk_crypto_dequeue(struct mtk_crypto_priv *priv)
{
	struct crypto_async_request *req;
	struct crypto_async_request *backlog;
	struct mtk_crypto_context *ctx;
	int ret;

	while (true) {
		spin_lock_bh(&priv->mtk_eip_queue.queue_lock);
		backlog = crypto_get_backlog(&priv->mtk_eip_queue.queue);
		req = crypto_dequeue_request(&priv->mtk_eip_queue.queue);
		spin_unlock_bh(&priv->mtk_eip_queue.queue_lock);

		if (!req)
			goto finalize;

		ctx = crypto_tfm_ctx(req->tfm);
		ret = ctx->send(req);
		if (ret)
			goto finalize;

		if (backlog)
			backlog->complete(backlog, -EINPROGRESS);
	}

finalize:
	return;
}

void mtk_crypto_dequeue_work(struct work_struct *work)
{
	struct mtk_crypto_work_data *data =
			container_of(work, struct mtk_crypto_work_data, work);
	mtk_crypto_dequeue(data->priv);
}
