# app/test1: TEST1.BIN, 目前直接用仓库里的成品, 不从 test1.s 构建

FSROOT_FILES += $(FSROOT)/TEST1.BIN
$(FSROOT)/TEST1.BIN: app/test1/TEST1.BIN
