default: all
##############################################################
include submodules/c_deps/etc/tools.mk
##############################################################
DIR=$(shell $(PWD))
M1_DIR=$(DIR)
LOADER_DIR=$(DIR)/loader
EMBEDS_DIR=$(DIR)/embeds
VENDOR_DIR=$(DIR)/vendor
PROJECT_DIR=$(DIR)
MESON_DEPS_DIR=$(DIR)/meson/deps
VENDOR_DIR=$(DIR)/vendor
DEPS_DIR=$(DIR)/deps
BUILD_DIR=$(DIR)/build
ETC_DIR=$(DIR)/etc
MENU_DIR=$(DIR)/menu
DOCKER_DIR=$(DIR)/docker
LIST_DIR=$(DIR)/list
SOURCE_VENV_CMD=$(DIR)/scripts
VENV_DIR=$(DIR)/.venv
SCRIPTS_DIR=$(DIR)/scripts
ACTIVE_APP_DIR=$(DIR)/active-window
SOURCE_VENV_CMD = source $(VENV_DIR)/bin/activate
##############################################################
TIDIED_FILES = \
			   */*.h */*.c
##############################################################
include submodules/c_deps/etc/includes.mk
all: build test

build-meson: 

do-meson: do-meson-build
meson-build: build-meson
build-meson: do-meson-build do-build
do-meson-build:
	@eval cd . && {  meson build || { meson build --reconfigure || { meson build --wipe; } && meson build; }; }
do-install: all
	@meson install -C build
do-build:
	@meson compile -C build
do-test:
	@passh meson test -C build -v --print-errorlogs
install: do-install
test: do-test
build: do-meson do-build muon
rm-make-logs:
	@rm .make-log* 2>/dev/null||true

dev-all: all

pull:
	@git pull


dev: nodemon
nodemon:
	@$(NODEMON) \
		--delay .3 \
		-w "*/*.c" -w "*/*.h" -w Makefle -w "*/meson.build" \
		-e c,h,sh,Makefile,build \
		-i build \
		-i build -i build-meson \
		-i submodules \
		-i .utils \
		-i .git \
			-x sh -- --norc --noprofile -c 'clear;make||true'
do-pull-submodules-cmds:
	@command find submodules -type d -maxdepth 1|xargs -I % echo -e "sh -c 'cd % && git pull'"
run-binary:
	@clear; make meson-binaries | env FZF_DEFAULT_COMMAND= \
		fzf --reverse \
			--preview-window='follow,wrap,right,80%' \
			--bind 'ctrl-b:preview(make meson-build)' \
			--preview='env bash -c {} -v -a' \
			--ansi --border \
			--cycle \
			--header='Select Test Binary' \
			--height='100%' \
	| xargs -I % env bash -c "./%"
run-binary-nodemon:
	@make meson-binaries | fzf --reverse | xargs -I % nodemon -w build --delay 1000 -x passh "./%"
meson-tests-list:
	@meson test -C build --list
meson-tests:
	@{ make meson-tests-list; } |fzf \
		--reverse \
		--bind 'ctrl-b:preview(make meson-build)' \
		--bind 'ctrl-t:preview(make meson-tests-list)'\
		--bind 'ctrl-l:reload(make meson-tests-list)'\
		--bind 'ctrl-k:preview(make clean meson-build)'\
		--bind 'ctrl-y:preview-half-page-up'\
		--bind 'ctrl-u:preview-half-page-down'\
		--bind 'ctrl-/:change-preview-window(right,80%|down,90%,border-horizontal)' \
		--preview='\
			meson test --num-processes 1 -C build -v \
				--no-stdsplit --print-errorlogs {}' \
		--preview-window='follow,wrap,bottom,85%' \
        --ansi \
		--header='Select Test Case |ctrl+b:rebuild project|ctrl+k:clean build|ctrl+t:list tests|ctrl+l:reload test list|ctrl+/:toggle preview style|ctrl+y/u:scroll preview|'\
		--header-lines=0 \
        --height='100%'

keybinds-yaml-to-json:
	@cat etc/keybinds.yaml|yaml2json |jq |tee etc/keybinds.json
