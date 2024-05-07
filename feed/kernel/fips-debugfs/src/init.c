// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2023 Mediatek Inc. All Rights Reserved.
 *
 * Author: Ren-Ting Wang <ren-ting.wang@mediatek.com>
 */

#include <linux/device.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/scatterlist.h>
#include <linux/uaccess.h>
#include <crypto/aead.h>
#include <crypto/skcipher.h>

#include "fips.h"

struct dentry *debug_root;
char key[32];
char iv[16];
char text[8500];
char assoc[8500];
char result[16384];
unsigned int klen;
unsigned int ivsize;
unsigned int msglen;
unsigned int alen;
unsigned int authsize;
int resultsize;

static int mtkcrypto_fips_3des_test(int mode)
{
	struct crypto_skcipher *tfm;
	struct skcipher_request *req = NULL;
	struct scatterlist src, dst;
	char *input = NULL;
	char *output = NULL;
	int ret = 0;
	DECLARE_CRYPTO_WAIT(wait);

	if (mode >= 6)
		tfm = crypto_alloc_skcipher("safexcel-cbc-des3_ede", 0, 0);
	else
		tfm = crypto_alloc_skcipher("safexcel-ecb-des3_ede", 0, 0);

	if (IS_ERR(tfm)) {
		pr_err("skcipher: failed to allocate tfm\n");
		return PTR_ERR(tfm);
	}

	req = skcipher_request_alloc(tfm, GFP_KERNEL);
	if (!req) {
		pr_err("skcipher: failed to allocate request\n");
		ret = -ENOMEM;
		goto out;
	}

	ret = crypto_skcipher_setkey(tfm, key, klen);
	if (ret) {
		pr_err("skcipher: setkey failed\n");
		goto out;
	}

	// Prepare input
	input = kmalloc(msglen, GFP_KERNEL);
	if (!input)
		goto out;

	memset(input, 0, msglen);
	memcpy(input, text, msglen);
	sg_init_one(&src, input, msglen);

	output = kmalloc(msglen, GFP_KERNEL);
	if (!output)
		goto out;

	memset(output, 0, msglen);
	sg_init_one(&dst, output, msglen);

	skcipher_request_set_callback(req, CRYPTO_TFM_REQ_MAY_BACKLOG, crypto_req_done, &wait);
	skcipher_request_set_crypt(req, &src, &dst, msglen, iv);
	ret = (mode % 2 == 0) ? crypto_skcipher_encrypt(req) : crypto_skcipher_decrypt(req);
	ret = crypto_wait_req(ret, &wait);
	if (ret) {
		pr_notice("aead failed to encrypt/decrypt: %d\n", ret);
		resultsize = ret;
		goto out;
	}

	memcpy(result, output, msglen);
	resultsize = msglen;

	ret = 0;
out:
	skcipher_request_free(req);
	crypto_free_skcipher(tfm);
	kfree(input);
	kfree(output);

	return ret;
}

static ssize_t mtkcrypto_fips_debug_write(struct file *file, const char __user *buffer,
					  size_t count, loff_t *data)
{
	struct crypto_aead *tfm;
	struct aead_request *req = NULL;
	struct scatterlist src, dst;
	char buf[4];
	int ret;
	char *input = NULL;
	char *output = NULL;
	unsigned int data_size = 0;
	u32 mode; // 0 = gcm enc, 1 = gcm dec, 2 = ccm enc, 3 = ccm dec, 4 = 3des enc, 5 = 3des dec
	DECLARE_CRYPTO_WAIT(wait);

	if (count > 2)
		return -EINVAL;

	if (copy_from_user(buf, buffer, count))
		return -EFAULT;

	if (kstrtou32(buf, 0, &mode))
		return -EINVAL;

	if (mode >= 4) {
		ret = mtkcrypto_fips_3des_test(mode);
		if (ret == 0)
			ret = count;
		return ret;
	}

	// Initialize crypto transform entity
	if (mode < 2)
		tfm = crypto_alloc_aead("safexcel-gcm-aes", 0, 0);
	else
		tfm = crypto_alloc_aead("safexcel-ccm-aes", 0, 0);
	if (IS_ERR(tfm)) {
		pr_err("aead: failed to allocate transform\n");
		return PTR_ERR(tfm);
	}

	req = aead_request_alloc(tfm, GFP_KERNEL);
	if (!req) {
		pr_err("aead: failed to allocate request\n");
		ret = -ENOMEM;
		goto crypto_free;
	}

	ret = crypto_aead_setkey(tfm, key, klen);
	if (ret) {
		pr_err("aead setkey failed\n");
		goto crypto_free;
	}

	ret = crypto_aead_setauthsize(tfm, authsize);
	if (ret) {
		pr_err("aead setauthsize failed\n");
		goto crypto_free;
	}

	// Prepare input
	input = kmalloc(alen + msglen, GFP_KERNEL);
	if (!input)
		goto crypto_free;

	memset(input, 0, alen + msglen);
	memcpy(input, assoc, alen);
	memcpy(input + alen, text, msglen);
	sg_init_one(&src, input, alen + msglen);

	// Prepare output
	if (mode % 2 == 0) {
		data_size = alen + msglen + authsize;
		pr_notice("enc, data_size: %d\n", data_size);
	} else {
		data_size = alen + msglen - authsize;
		pr_notice("dec, data_size: %d\n", data_size);
	}
	output = kmalloc(data_size, GFP_KERNEL);
	if (!output)
		goto crypto_free;

	memset(output, 0, data_size);
	sg_init_one(&dst, output, data_size);

	// Setup request for crypto API
	aead_request_set_callback(req, CRYPTO_TFM_REQ_MAY_BACKLOG, crypto_req_done, &wait);
	aead_request_set_crypt(req, &src, &dst, msglen, iv);
	aead_request_set_ad(req, alen);
	ret = (mode % 2) ? crypto_aead_decrypt(req) : crypto_aead_encrypt(req);
	ret = crypto_wait_req(ret, &wait);
	if (ret) {
		pr_notice("aead failed to encrypt/decrypt: %d\n", ret);
		resultsize = ret;
		goto crypto_free;
	}

	memcpy(result, output + alen, data_size - alen);
	resultsize = data_size - alen;

	ret = count;
crypto_free:
	aead_request_free(req);
	crypto_free_aead(tfm);
	kfree(input);
	kfree(output);

	return ret;
}

static int mtkcrypto_debug_read(struct seq_file *s, void *private)
{
	return 0;
}

static int mtkcrypto_fips_debug_open(struct inode *inode, struct file *file)
{
	return single_open(file, mtkcrypto_debug_read, file->private_data);
}

static const struct file_operations mtkcrypto_debug_ops = {
	.open = mtkcrypto_fips_debug_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = mtkcrypto_fips_debug_write,
	.release = single_release,
};

static int __init mtkcrypto_debugfs_init(void)
{
	debug_root = debugfs_create_dir("fips", NULL);
	if (!debug_root) {
		pr_err("create debugfs root directory failed\n");
		return -ENOMEM;
	}

	mtkcrypto_key_init();
	mtkcrypto_iv_init();
	mtkcrypto_msg_init();
	mtkcrypto_assoc_init();
	mtkcrypto_result_init();
	mtkcrypto_tagsize_init();

	debugfs_create_file("test", 0444, debug_root, NULL,
			    &mtkcrypto_debug_ops);
	return 0;
}

static void __exit mtkcrypto_debugfs_exit(void)
{
	debugfs_remove_recursive(debug_root);
	debug_root = NULL;
}

module_init(mtkcrypto_debugfs_init);
module_exit(mtkcrypto_debugfs_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MediaTek FIPS interface");
MODULE_AUTHOR("Chris Chou <chris.chou@mediatek.com>");
