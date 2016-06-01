proj := qrtr
proj-major := 1
proj-minor := 0
proj-version := $(proj-major).$(proj-minor)

CFLAGS := -Wall -g
LDFLAGS :=

ifeq ($(PREFIX),)
PREFIX := /usr
endif

ifneq ($(CROSS_COMPILE),)
CC := $(CROSS_COMPILE)gcc
endif
SFLAGS := -I$(shell $(CC) -print-file-name=include) -Wno-non-pointer-null

$(proj)-cfg-srcs := \
	src/cfg.c

$(proj)-ns-srcs := \
	src/ns.c \
	src/map.c \
	src/hash.c \
	src/waiter.c \
	src/util.c \

$(proj)-lookup-srcs := \
	src/lookup.c \
	src/util.c \

lib$(proj).so-srcs := \
	lib/libqrtr.c \

lib$(proj).so-cflags := -fPIC -Isrc

targets := $(proj)-ns $(proj)-cfg $(proj)-lookup lib$(proj).so

out := out
src_to_obj = $(patsubst %.c,$(out)/obj/%.o,$(1))
src_to_dep = $(patsubst %.c,$(out)/dep/%.d,$(1))

all-srcs :=
all-objs :=
all-deps :=
all-clean := $(out)
all-install :=

all: $(targets)

$(out)/obj/%.o: %.c
ifneq ($C,)
	@echo "CHECK	$<"
	@sparse $< $(patsubst -iquote=%,-I%,$(CFLAGS)) $(SFLAGS)
endif
	@echo "CC	$<"
	@$(CC) -MM -MF $(call src_to_dep,$<) -MP -MT "$@ $(call src_to_dep,$<)" $(CFLAGS) $(_CFLAGS) $<
	@$(CC) -o $@ -c $< $(CFLAGS) $(_CFLAGS)

define add-target
all-srcs += $($1-srcs)
all-objs += $(call src_to_obj,$($1-srcs))
all-deps += $(call src_to_dep,$($1-srcs))
all-clean += $1
$(call src_to_obj,$($1-srcs)): _CFLAGS := $($1-cflags)

$1: $(call src_to_obj,$($1-srcs))
	@echo "LD	$$@"
	@$$(CC) -o $$@ $$(filter %.o,$$^) $(LDFLAGS) $2

$3: $1
	@echo "INSTALL	$$<"
	@install -m 755 $$< $$@

all-install += $3
endef
add-bin-target = $(call add-target,$1,-static,$(PREFIX)/bin/$1)
add-lib-target = $(call add-target,$1,-shared,$(PREFIX)/lib/$1)

$(foreach v,$(filter-out %.so,$(targets)),$(eval $(call add-bin-target,$v)))
$(foreach v,$(filter %.so,$(targets)),$(eval $(call add-lib-target,$v)))

install: $(all-install)

clean:
	@echo CLEAN
	@$(RM) -r $(all-clean)

$(call src_to_obj,$(all-srcs)): Makefile

ifneq ("$(MAKECMDGOALS)","clean")
cmd-goal-1 := $(shell mkdir -p $(sort $(dir $(all-objs) $(all-deps))))
-include $(all-deps)
endif
