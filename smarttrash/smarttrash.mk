################################################################################
# Buildroot Makefile for the SMARTTRASH application
################################################################################

SMARTTRASH_VERSION = 1.0
SMARTTRASH_SITE = /home/nathalia/labs_tpse/buildroot/package/smarttrash
SMARTTRASH_LICENSE = MIT
SMARTTRASH_SITE_METHOD = local

# Comandos para compilar a aplicação
define SMARTTRASH_BUILD_CMDS
    $(TARGET_CC) $(TARGET_CFLAGS) -o $(@D)/smarttrash $(SMARTTRASH_SITE)/main.c
endef

# Comandos para instalar o binário no sistema de arquivos
define SMARTTRASH_INSTALL_TARGET_CMDS
    $(INSTALL) -D -m 0755 $(@D)/smarttrash $(TARGET_DIR)/usr/bin/smarttrash
endef

# Inclui o pacote no Buildroot
$(eval $(generic-package))

