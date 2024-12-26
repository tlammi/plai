
MESON_CMD ?= meson setup

ifeq ($(STATIC_LIB),1)
	MESON_CMD := $(MESON_CMD) -Ddefault_library=static
endif

ifeq ($(SANITIZE_ADDRESS),1)
	MESON_CMD := $(MESON_CMD) -Db_sanitize=address
endif

ifeq ($(SANITIZE_THREAD),1)
	MESON_CMD := $(MESON_CMD) -Db_sanitize=thread
endif

.PHONY: compile
compile: BARGS=meson compile
compile: .in-build

TEST_NAME ?= *
.PHONY: test
test: BARGS=meson test "${TEST_NAME}"
test: .in-build

.PHONY: build
build: build/tag

.PHONY: clean
clean:
	rm -rf build

build/tag:
	$(MESON_CMD) build
	touch build/tag


.in-build: build
	cd build && ${BARGS}

