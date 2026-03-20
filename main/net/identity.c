#include "identity.h"
#include "esp_err.h"
#include "esp_mac.h"
#include "esp_random.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/entropy.h"
#include "mbedtls/sha256.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <string.h>

static const char *NVS_NAMESPACE = "net_identity";

esp_err_t identity_init(node_identity_t *id) {
    esp_err_t err;
    nvs_handle_t nvs_handle;
    size_t key_size = PRIVKEY_LEN;

    err = nvs_flash_init();
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_get_blob(nvs_handle, "priv_key", id->private_key, &key_size);

    if (err == ESP_OK) {
        key_size = PUBKEY_LEN;
        nvs_get_blob(nvs_handle, "pub_key", id->public_key, &key_size);
        key_size = NODE_ID_LEN;
        nvs_get_blob(nvs_handle, "node_id", id->node_id, &key_size);
    } else {
        mbedtls_ctr_drbg_context ctr_drbg;
        mbedtls_entropy_context entropy;
        mbedtls_ecdsa_context ecdsa_ctx;

        mbedtls_entropy_init(&entropy);
        mbedtls_ctr_drbg_init(&ctr_drbg);
        mbedtls_ecdsa_init(&ecdsa_ctx);

        unsigned char seed[32];
        for (int i = 0; i < 32; i++) {
            seed[i] = esp_random() & 0xFF;
        }

        mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                              seed, sizeof(seed));

        mbedtls_ecdsa_genkey(&ecdsa_ctx, MBEDTLS_ECP_DP_SECP256K1,
                             mbedtls_ctr_drbg_random, &ctr_drbg);

        size_t olen;
        mbedtls_ecp_write_public_key(&ecdsa_ctx, 0, &olen,
                                     id->public_key, PUBKEY_LEN);
        mbedtls_ecp_write_key_ext(&ecdsa_ctx, &olen,
                                  id->private_key, PRIVKEY_LEN);

        mbedtls_sha256(id->public_key, PUBKEY_LEN, id->node_id, 0);

        nvs_set_blob(nvs_handle, "priv_key", id->private_key, PRIVKEY_LEN);
        nvs_set_blob(nvs_handle, "pub_key", id->public_key, PUBKEY_LEN);
        nvs_set_blob(nvs_handle, "node_id", id->node_id, NODE_ID_LEN);
        nvs_commit(nvs_handle);

        mbedtls_ecdsa_free(&ecdsa_ctx);
        mbedtls_ctr_drbg_free(&ctr_drbg);
        mbedtls_entropy_free(&entropy);
    }

    nvs_close(nvs_handle);
    return ESP_OK;
}

esp_err_t identity_sign(const node_identity_t *id, const uint8_t *msg,
                        size_t msg_len, uint8_t *signature) {
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;
    mbedtls_ecdsa_context ecdsa_ctx;
    int ret;

    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_ecdsa_init(&ecdsa_ctx);

    unsigned char seed[32];
    for (int i = 0; i < 32; i++) {
        seed[i] = esp_random() & 0xFF;
    }

    mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                          seed, sizeof(seed));

    ret = mbedtls_ecp_read_key(MBEDTLS_ECP_DP_SECP256K1, &ecdsa_ctx,
                               id->private_key, PRIVKEY_LEN);
    if (ret != 0) {
        goto cleanup;
    }

    size_t sig_len = SIG_LEN;
    ret = mbedtls_ecdsa_write_signature(&ecdsa_ctx, MBEDTLS_MD_SHA256,
                                        msg, msg_len, signature, SIG_LEN,
                                        &sig_len, mbedtls_ctr_drbg_random,
                                        &ctr_drbg);

cleanup:
    mbedtls_ecdsa_free(&ecdsa_ctx);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);

    return (ret == 0) ? ESP_OK : ESP_FAIL;
}

esp_err_t identity_verify(const uint8_t *public_key,
                          const uint8_t *msg, size_t msg_len,
                          const uint8_t *signature) {
    mbedtls_ecdsa_context ecdsa_ctx;
    int ret;

    mbedtls_ecdsa_init(&ecdsa_ctx);

    ret = mbedtls_ecp_read_key(MBEDTLS_ECP_DP_SECP256K1, &ecdsa_ctx,
                               public_key, PUBKEY_LEN);
    if (ret != 0) {
        goto cleanup;
    }

    ret = mbedtls_ecdsa_read_signature(&ecdsa_ctx, msg, msg_len,
                                       signature, SIG_LEN);

cleanup:
    mbedtls_ecdsa_free(&ecdsa_ctx);

    return (ret == 0) ? ESP_OK : ESP_FAIL;
}
