SOURCES = source/sskd.c source/ssmk.c source/sscl.c source/ssct.c source/ssls.c
HEADERS = source/sskit.h

INFO = $(firstword $(HEADERS))

include mk/binary.mk
include mk/arch.mk
include mk/debian.mk
include mk/ocs.mk
include mk/man.mk
include mk/conf.mk
include mk/dist.mk
