#ifndef CERTS_H
#define CERTS_H

// Example only. Prefer demo/aws/provision-thing.sh which writes certs.h.
// Do not commit real certificates.

static const char AWS_CERT_CA[] =
    "-----BEGIN CERTIFICATE-----\n"
    "REPLACE_WITH_AMAZON_ROOT_CA_1_CONTENT\n"
    "-----END CERTIFICATE-----\n";

static const char AWS_CERT_CRT[] =
    "-----BEGIN CERTIFICATE-----\n"
    "REPLACE_WITH_DEVICE_CERTIFICATE_CONTENT\n"
    "-----END CERTIFICATE-----\n";

static const char AWS_CERT_PRIVATE[] =
    "-----BEGIN RSA PRIVATE KEY-----\n"
    "REPLACE_WITH_DEVICE_PRIVATE_KEY_CONTENT\n"
    "-----END RSA PRIVATE KEY-----\n";

#endif  // CERTS_H
