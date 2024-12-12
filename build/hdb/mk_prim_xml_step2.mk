# include $(MAIN_TOPDIR)/build/hdb/mk_prim_xml.rules

include $(MAIN_TOPDIR)/build_tmp/config/soc_product_config.mk

ifeq ($(PRODUCT_CFG_BUILD_RELEASE), yes)
MAIN_OUT_DIR_NAME = $(project_name)_release
else
MAIN_OUT_DIR_NAME = $(project_name)_debug
endif
MAIN_BUILD_TMP_OUT := $(RELATIVE_DIR)/build_tmp/$(MAIN_OUT_DIR_NAME)


PRIM_XML_TEMP_BASE_ROOT_DIR:=$(MAIN_OUT)/TEMP_DIR/$(HB_PRODUCT_NAME_LIST)/PRIM/base
PRIM_XML_TEMP_ROOT_DIR:=$(MAIN_OUT)/TEMP_DIR/$(HB_PRODUCT_NAME_LIST)/PRIM
PRIM_XML_DST_XML_FILE:=$(MAIN_OUT)/$(HB_PRODUCT_NAME_LIST)/hdb/hdbcfg/mss_prim_db.xml
PRIM_XML_SRC_XML_FILE:=$(MAIN_TOPDIR)/target/config/hdbcfg/mss_prim_db.xml


PYTHON_ARGS_DO_PRIM_XML_STEP2 = "_PYTHON_ARG_" "PRIM_XML_TEMP_BASE_ROOT_DIR" $(PRIM_XML_TEMP_BASE_ROOT_DIR)\
	"_PYTHON_ARG_" "PRIM_XML_DST_XML_FILE" $(PRIM_XML_DST_XML_FILE)\
	"_PYTHON_ARG_" "PRIM_XML_SRC_XML_FILE" $(PRIM_XML_SRC_XML_FILE)\
	"_PYTHON_ARG_" "LOG_FILE_NAME" $(PRIM_XML_TEMP_ROOT_DIR)/LOG/BASE.log
		
do_prim_xml_step2:
#@echo do_prim_xml_step2 PYTHON_ARGS_DO_PRIM_XML_STEP2=$(PYTHON_ARGS_DO_PRIM_XML_STEP2)
	@python3 $(MAIN_TOPDIR)/build/hdb/mk_prim_xml_step2.py $(PYTHON_ARGS_DO_PRIM_XML_STEP2)
