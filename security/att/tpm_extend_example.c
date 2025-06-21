#include <linux/module.h>       // Required for all kernel modules (init/exit macros, MODULE_* macros)
#include <linux/init.h>         // For module_init and module_exit macros
#include <linux/string.h>       // For string handling like strlen, memcpy
#include <linux/tpm.h>          // Core TPM interface: tpm_chip, tpm_pcr_extend(), tpm_try_get_ops(), etc.
#include <linux/crypto.h>       // For crypto_alloc_shash and related cryptographic interfaces
#include <crypto/hash.h>        // For hashing (shash_desc, crypto_shash_digest, etc.)
#include "tpm_extend_example.h"
#include "att_queue.h"

#define PCR_INDEX 23
#define SHA256_DIGEST_SIZE 32

// Kthread task strucutre (the thread that actually extends events into the TPM)
static struct task_struct *att_thread;

int __init tpm_extend_example_init(void)
{


	/* TODO: Move the below code to its separate module */
	/* The below code should be executed in late_initcall stage. */
	int ret;
    ret = kfifo_alloc(&att_fifo, ATT_FIFO_SIZE * sizeof(unsigned char[ATT_MAX_MSG_SIZE]), GFP_KERNEL);
    if (ret)
        return ret;

    att_thread = kthread_run(att_worker_thread, NULL, "att_secure_worker");
    if (IS_ERR(att_thread)) {
        kfifo_free(&att_fifo);
        return PTR_ERR(att_thread);
    }
	/* TODO: Move the above code */

	struct tpm_chip *chip;
	struct tpm_digest *digests;
	const char *data = "Hello World";
	struct crypto_shash *tfm = NULL;
	struct shash_desc *desc = NULL;
	u8 hash[SHA256_DIGEST_SIZE];
	int i, rc = 0;
	bool sha256_found = false;

	chip = tpm_default_chip();
	if (!chip) {
		pr_err("TPM chip not found\n");
		return -ENODEV;
	}

	// Allocate digest array for all banks
	digests = kcalloc(chip->nr_allocated_banks, sizeof(*digests), GFP_KERNEL);
	if (!digests) {
		pr_err("Failed to allocate digest array\n");
		return -ENOMEM;
	}

	// Compute SHA-256 hash of "Hello World"
	tfm = crypto_alloc_shash("sha256", 0, 0);
	if (IS_ERR(tfm)) {
		pr_err("SHA-256 not supported in kernel\n");
		rc = PTR_ERR(tfm);
		goto out_free;
	}

	desc = kmalloc(sizeof(*desc) + crypto_shash_descsize(tfm), GFP_KERNEL);
	if (!desc) {
		rc = -ENOMEM;
		goto out_free;
	}

	desc->tfm = tfm;

	rc = crypto_shash_digest(desc, data, strlen(data), hash);
	if (rc) {
		pr_err("Failed to compute SHA-256 hash\n");
		goto out_free;
	}

	// Fill digests[]:
	// - SHA-256 → real hash
	// - Others  → zero-fill
	for (i = 0; i < chip->nr_allocated_banks; i++) {
		u16 alg_id = chip->allocated_banks[i].alg_id;
		digests[i].alg_id = alg_id;

		if (alg_id == TPM_ALG_SHA256) {
			memcpy(digests[i].digest, hash, SHA256_DIGEST_SIZE);
			sha256_found = true;
		} else {
			memset(digests[i].digest, 0, TPM_MAX_DIGEST_SIZE);
		}
	}

	if (!sha256_found) {
		pr_err("TPM does not support SHA-256 bank\n");
		rc = -EINVAL;
		goto out_free;
	}

	// Extend to PCR
	rc = tpm_pcr_extend(chip, PCR_INDEX, digests);
	if (rc)
		pr_err("tpm_pcr_extend failed: %d\n", rc);
	else
		pr_info("Successfully extended PCR[%d] with SHA-256('Hello World')\n", PCR_INDEX);

out_free:
	kfree(desc);
	if (tfm && !IS_ERR(tfm))
		crypto_free_shash(tfm);
	kfree(digests);
	return rc;
}
