# WHERE'S THE STUFF
SOURCEDIR := Source
LIBDIR := Libraries

# The name of the project (names the compiled binary)
BINARY_NAME = code

# Python 2.7
PYTHON_INCLUDE = /usr/include/python2.7

# COMPILER OPTIONS
COMPILE_OPTIONS = -pthread -fopenmp -lpython2.7
LINK_OPTIONS = -fopenmp -lpython2.7

#************************************************************************
# Support for Additional compile-time parameters
#************************************************************************
# override directive utilized for text assertions printed to end-user
# https://www.gnu.org/software/make/manual/make.html#Override-Directive

# [BUILD TYPE]
# Defaults to debug build configuration if not specified
# Supported options: debug, internal, release
# https://gcc.gnu.org/onlinedocs/gcc-4.2.4/gcc/Debugging-Options.html#Debugging-Options
# DO NOT EDIT BUILD_FILE without consideration of "clean" recipie
ifeq ($(build),release)
	override build = RELEASE
	LD_BUILD = 
	CPP_DEBUG = -g0
else
	ifeq ($(build),internal)
		override build = INTERNAL
		LD_BUILD = 
		CPP_DEBUG = -g
	else
		# DEFAULT CASE
		override build = DEBUG
		LD_BUILD = -Xlinker -M=$(BINARY_NAME).map -Xlinker --cref
		CPP_DEBUG = -pedantic -Wextra -Wconversion -g3
	endif
endif

# [OPTIMIZATIONS]
# Defaults to NO optimizations
# Supported options: O0, O1, O2, O3
ifeq ($(optimize),O3)
	override optimize = O3
	COMPILE_OPTIMIZATION = -O3
else
	ifeq ($(optimize),O2)
		override optimize = O2
		COMPILE_OPTIMIZATION = -O2
	else
		ifeq ($(optimize),O1)
			override optimize = O1
			COMPILE_OPTIMIZATION = -O1
		else
			# DEFAULT CASE
			override optimize = O0
			COMPILE_OPTIMIZATION = -O0
		endif
	endif
endif

#************************************************************************
# Settings below this point usually do not need to be edited
#************************************************************************

# [TERMINAL COLORIZATION]
COLOR_INFO_PRE = \033[4;94m
COLOR_INFO_POST = \033[0m

# [WINDOWS] Adjust for target build environments (force .exe suffix)
# NOTE (1): See LDFLAGS variable for additional notes
ifeq ($(OS),Windows_NT)
	OS_LINKER = -Xlinker --force-exe-suffix
endif

# Automatically create lists of the sources and objects

# VPATH
VPATH=$(SOURCEDIR):$(shell find $(LIBDIR) -maxdepth 1 -type d -printf '%f:')

#### GNU MAKE VARIABLES ####
# https://www.gnu.org/software/make/manual/html_node/Implicit-Variables.html

# [COMMON] "Extra flags to give to the C preprocessor and programs that use it (the C and Fortran compilers)."
CPPFLAGS = -Wall $(CPP_DEBUG) $(COMPILE_OPTIMIZATION) -MMD $(COMPILE_OPTIONS) -I$(SOURCEDIR) $(shell find $(LIBDIR) -maxdepth 1 -type d -printf ' -I%p') -I$(PYTHON_INCLUDE)

# [CPP] "Extra flags to give to the C++ compiler."
CXXFLAGS = -fPIC -std=c++17 -fno-exceptions -fno-elide-constructors

# [C] "Extra flags to give to the C compiler."
CFLAGS =

# [LINKER] "Extra flags to give to compilers when they are supposed to invoke the linker"
LDFLAGS = -fPIC $(LD_BUILD) $(LINK_OPTIONS) -Xlinker --warn-common $(OS_LINKER)
# [1] -Xlinker necessary for passing options to linker directly
# see: https://gcc.gnu.org/onlinedocs/gcc-4.6.1/gcc/Link-Options.html for details

# [LINKER] "Library flags or names given to compilers when they are supposed to invoke the linker"
#LDLIBS = 
#LD_LIBRARY_PATH =

#### TOOLCHAIN ####
# Names for the compiler programs
CC = gcc
CXX = g++
LD = ld

# Version query commands
CC_VER = $(CC) $(shell $(CC) -dumpversion)
CXX_VER = $(CXX) $(shell $(CXX) -dumpversion)
LD_VER = $(shell $(LD) -v)

#### FIND ALL THE SOURCES ####
# Find Source Files in Source and Library Directory
C_FILES := $(shell find $(SOURCEDIR) -name '*.c')
CPP_FILES := $(shell find $(SOURCEDIR) -name '*.cpp')

C_LIBS := $(shell find $(LIBDIR) -name '*.c')
CPP_LIBS := $(shell find $(LIBDIR) -name '*.cpp')
OBJECTS := $(C_LIBS:.c=.o) $(C_FILES:.c=.o)
OBJECTS += $(CPP_LIBS:.cpp=.o) $(CPP_FILES:.cpp=.o)

#### MAKE RECIPIES ####

#.PHONY targets
.PHONY: all build-log clean dirs dummy-all from-source from-src info version\
	commit-active commit-dev commit-master git-branch-dev git-branch-master git-init\
	changes release-dev

all: | version dummy-all
	@printf "\n$(COLOR_INFO_PRE)($$(date --rfc-3339=seconds)) [COMPLETE]$(COLOR_INFO_POST)\n"
	
build-log:
	@rm -f BUILD
	@printf "($$(date --rfc-3339=seconds)) [BUILD]" >> BUILD
	@printf "\nBuild Information:" >> BUILD
	@printf "\n\tBuild Type: $(build)" >> BUILD
	@printf "\n\tOptimization: $(optimize)" >> BUILD
	@printf "\n\tTarget OS: $(OS)" >> BUILD
	@printf "\nBuild Tools:" >> BUILD
	@printf "\n\tCompiler (C): $(CC_VER)" >> BUILD
	@printf "\n\tCompiler (CPP): $(CXX_VER)" >> BUILD
	@printf "\n\tLinker: $(LD_VER)" >> BUILD

clean:
	@printf "\n$(COLOR_INFO_PRE)($$(date --rfc-3339=seconds)) [CLEAN]$(COLOR_INFO_POST)\n"
	@find . -type f -name "*.o" -delete
	@find . -type f -name "*.d" -delete
	@rm -f $(BINARY_NAME).map
	@rm -f $(BINARY_NAME) $(BINARY_NAME).exe
	@rm -f $(BINARY_NAME).stackdump $(BINARY_NAME).exe.stackdump
	@rm -f BUILD

dirs:
	@mkdir -p $(SOURCEDIR)
	@mkdir -p $(LIBDIR)
	
dummy-all: | info $(BINARY_NAME)
# Order-dependent to ensure information print-out is at top of console

from-source: | clean build-log dummy-all
# Order-dependent to ensure any existing compiled content is cleaned first.

from-src: from-source
# Name Alias

info:
	@printf "\n$(COLOR_INFO_PRE)($$(date --rfc-3339=seconds)) [MAKE]$(COLOR_INFO_POST)"
	@printf "\nBuild Information:"
	@printf "\n\tBuild Type: $(build)"
	@printf "\n\tOptimization: $(optimize)"
	@printf "\n\tTarget OS: $(OS)"
	@printf "\nBuild Tools:"
	@printf "\n\tCompiler (C): $(CC_VER)"
	@printf "\n\tCompiler (CPP): $(CXX_VER)"
	@printf "\n\tLinker: $(LD_VER)"
	@printf "\n"

version:
	@printf "\n$(COLOR_INFO_PRE)($$(date --rfc-3339=seconds)) [VERSION CHECK]$(COLOR_INFO_POST)\n"
	@if $$(grep -q "$(build)" BUILD) && $$(grep -q "$(optimize)" BUILD); then\
		printf "Build version and optimization are consistent.\n";\
	else\
		make clean;\
		make build-log;\
	fi

#### ACTUAL INTERACTION WITH CODE (STARTS HERE) ####
$(BINARY_NAME): $(OBJECTS)
	@printf "\n$(COLOR_INFO_PRE)($$(date --rfc-3339=seconds)) [LINK BINARY] $@$(COLOR_INFO_POST)\n"
	$(CXX) $(CPP_DEBUG) -o $@ $(OBJECTS) $(LDFLAGS)
#  -Xlinker -I$(PYTHON_INCLUDE) -lpython2.7
	@# Everything goes to hell if CC is called, not CXX

#### IMPLICIT RULES (DEFINED) ####
# Re-Define C for added verbosity
# Explicit use of (CXX) necessary for Cygwin compatibility
%.o : %.c
	@printf "\n$(COLOR_INFO_PRE)($$(date --rfc-3339=seconds)) [COMPILE C] $@$(COLOR_INFO_POST)\n"
	$(CC) -c $< $(CPPFLAGS) $(CFLAGS) -o $@
	
# Re-Define CPP for added verbosity [1/4]
%.o : %.cpp
	@printf "\n$(COLOR_INFO_PRE)($$(date --rfc-3339=seconds)) [COMPILE CPP] $@$(COLOR_INFO_POST)\n"
	$(CXX) -c $< $(CPPFLAGS) $(CXXFLAGS) -o $@

# Re-Define CPP for added verbosity [2/4]
%.o : %.cc
	@printf "\n$(COLOR_INFO_PRE)($$(date --rfc-3339=seconds)) [COMPILE CPP] $@$(COLOR_INFO_POST)\n"
	$(CXX) -c $< $(CPPFLAGS) $(CXXFLAGS) -o $@

# Re-Define CPP for added verbosity [3/4]
%.o : %.cxx
	@printf "\n$(COLOR_INFO_PRE)($$(date --rfc-3339=seconds)) [COMPILE CPP] $@$(COLOR_INFO_POST)\n"
	$(CXX) -c $< $(CPPFLAGS) $(CXXFLAGS) -o $@

# Re-Define CPP for added verbosity [4/4]
%.o : %.C
	@printf "\n$(COLOR_INFO_PRE)($$(date --rfc-3339=seconds)) [COMPILE CPP] $@$(COLOR_INFO_POST)\n"
	$(CXX) -c $< $(CPPFLAGS) $(CXXFLAGS) -o $@

#### Git Interface Functionality ####
# Wrappter for git differencing utilities with a few parameters
changes:
	@printf "\n$(COLOR_INFO_PRE)($$(date --rfc-3339=seconds)) [GIT LOG] $(COLOR_INFO_POST)\n"
	git log --stat -3
	git diff HEAD

# Initialize Directory as GIT Repository
commit-active:
	@printf "\n$(COLOR_INFO_PRE)($$(date --rfc-3339=seconds)) [GIT COMMIT] $(COLOR_INFO_POST)\n"
	git add .
	git commit --all

# Changes Branch to Development Prior to Commit
commit-dev : git-branch-dev
	@printf "\n$(COLOR_INFO_PRE)($$(date --rfc-3339=seconds)) [GIT COMMIT DEV] $(COLOR_INFO_POST)\n"
	git add .
	git commit --all

# Changes Branch to Master Prior to Commit	
commit-master : git-branch-master
	@printf "\n$(COLOR_INFO_PRE)($$(date --rfc-3339=seconds)) [GIT COMMIT MASTER] $(COLOR_INFO_POST)\n"
	git add .
	git commit --all
	
release-dev : git-branch-master
	@printf "\n$(COLOR_INFO_PRE)($$(date --rfc-3339=seconds)) [GIT COMMIT MASTER] $(COLOR_INFO_POST)\n"
	git merge Development
	git checkout development
	
git-branch-dev:
	git checkout development

git-branch-master:
	git checkout master

git-init : dirs git-ignore
	@printf "\n$(COLOR_INFO_PRE)($$(date --rfc-3339=seconds)) [GIT INITIALIZATION] $(COLOR_INFO_POST)\n"
	git init
	git add .
	git commit
	git branch development
	
#=============================.gitignore RAW TEXT=============================#
git-ignore:
	@if [ ! -f .gitignore ]; then\
		printf "\n#Script Variables\nBUILD\n">.gitignore;\
		printf "\n# Created by https://www.gitignore.io/api/c,cuda,osx,git,c++,vim,emacs,eclipse,tex,windows,fortran,sublimetext,matlab\n">>.gitignore;\
		printf "\n### C ###\n# Prerequisites\n*.d\n\n# Object files\n*.o\n*.ko\n*.obj\n*.elf\n\n">>.gitignore;\
		printf "# Linker output\n*.ilk\n*.map\n*.exp\n\n# Precompiled Headers\n*.gch\n*.pch\n\n">>.gitignore;\
		printf "# Libraries\n*.lib\n*.a\n*.la\n*.lo\n\n# Shared objects (inc. Windows DLLs)\n">>.gitignore;\
		printf "*.dll\n*.so\n*.so.*\n*.dylib\n\n# Executables\n*.exe\n*.out\n*.app\n*.i*86\n">>.gitignore;\
		printf "*.x86_64\n*.hex\n\n# Debug files\n*.dSYM/\n*.su\n*.idb\n*.pdb\n\n">>.gitignore;\
		printf "# Kernel Module Compile Results\n*.mod*\n*.cmd\n.tmp_versions/\nmodules.order\n">>.gitignore;\
		printf "Module.symvers\nMkfile.old\ndkms.conf\n\n">>.gitignore;\
		printf "### C++ ###\n# Compiled Object files\n*.slo\n\n">>.gitignore;\
		printf "# Fortran module files\n*.mod\n*.smod\n\n# Compiled Static libraries\n*.lai\n\n">>.gitignore;\
		printf "### CUDA ###\n*.i\n*.ii\n*.gpu\n*.ptx\n*.cubin\n*.fatbin\n\n">>.gitignore;\
		printf "### Eclipse ###\n\n.metadata\nbin/\ntmp/\n*.tmp\n*.bak\n*.swp\n*~.nib\n">>.gitignore;\
		printf "local.properties\n.settings/\n.loadpath\n.recommenders\n\n">>.gitignore;\
		printf "# External tool builders\n.externalToolBuilders/\n\n">>.gitignore;\
		printf "# Locally stored \"Eclipse launch configurations\"\n*.launch\n\n">>.gitignore;\
		printf "# PyDev specific (Python IDE for Eclipse)\n*.pydevproject\n\n">>.gitignore;\
		printf "# CDT-specific (C/C++ Development Tooling)\n.cproject\n\n">>.gitignore;\
		printf "# Java annotation processor (APT)\n.factorypath\n\n">>.gitignore;\
		printf "# PDT-specific (PHP Development Tools)\n.buildpath\n\n">>.gitignore;\
		printf "# sbteclipse plugin\n.target\n\n# Tern plugin\n.tern-project\n\n">>.gitignore;\
		printf "# TeXlipse plugin\n.texlipse\n\n# STS (Spring Tool Suite)\n.springBeans\n\n">>.gitignore;\
		printf "# Code Recommenders\n.recommenders/\n\n">>.gitignore;\
		printf "# Scala IDE specific (Scala & Java development for Eclipse)\n">>.gitignore;\
		printf ".cache-main\n.scala_dependencies\n.worksheet\n\n">>.gitignore;\
		printf "### Eclipse Patch ###\n# Eclipse Core\n.project\n\n">>.gitignore;\
		printf "# JDT-specific (Eclipse Java Development Tools)\n.classpath\n\n">>.gitignore;\
		printf "### Emacs ###\n# -*- mode: gitignore; -*-\n*~\n\#*\#\n/.emacs.desktop\n">>.gitignore;\
		printf "/.emacs.desktop.lock\n*.elc\nauto-save-list\ntramp\n.\#*\n\n# Org-mode\n">>.gitignore;\
		printf ".org-id-locations\n*_archive\n\n# flymake-mode\n*_flymake.*\n\n# eshell files\n">>.gitignore;\
		printf "/eshell/history\n/eshell/lastdir\n\n# elpa packages\n/elpa/\n\n# reftex files\n">>.gitignore;\
		printf "*.rel\n\n# AUCTeX auto folder\n/auto/\n\n# cask packages\n.cask/\ndist/\n\n">>.gitignore;\
		printf "# Flycheck\nflycheck_*.el\n\n# server auth directory\n/server/\n\n">>.gitignore;\
		printf "# projectiles files\n.projectile\nprojectile-bookmarks.eld\n\n">>.gitignore;\
		printf "# directory configuration\n.dir-locals.el\n\n# saveplace\nplaces\n\n# url cache\n">>.gitignore;\
		printf "url/cache/\n\n# cedet\nede-projects.el\n\n# smex\nsmex-items\n\n">>.gitignore;\
		printf "# company-statistics\ncompany-statistics-cache.el\n\n# anaconda-mode\n">>.gitignore;\
		printf "anaconda-mode/\n\n">>.gitignore;\
		printf "### Git ###\n*.orig\n\n">>.gitignore;\
		printf "### Matlab ###\n# Windows default autosave extension\n*.asv\n\n# OSX / *nix default autosave extension\n">>.gitignore;\
		printf "*.m~\n\n# Compiled MEX binaries (all platforms)\n*.mex*\n\n# Simulink Code Generation\nslprj/\n\n">>.gitignore;\
		printf "# Session info\noctave-workspace\n\n# Simulink autosave extension\n*.autosave\n\n">>.gitignore;\
		printf "### OSX ###\n*.DS_Store\n.AppleDouble\n.LSOverride\n\n">>.gitignore;\
		printf "# Icon must end with two \\r\nIcon\n\n# Thumbnails\n._*\n\n">>.gitignore;\
		printf "# Files that might appear in the root of a volume\n.DocumentRevisions-V100\n">>.gitignore;\
		printf ".fseventsd\n.Spotlight-V100\n.TemporaryItems\n.Trashes\n.VolumeIcon.icns\n">>.gitignore;\
		printf ".com.apple.timemachine.donotpresent\n\n">>.gitignore;\
		printf "# Directories potentially created on remote AFP share\n.AppleDB\n">>.gitignore;\
		printf ".AppleDesktop\nNetwork Trash Folder\nTemporary Items\n.apdisk\n\n">>.gitignore;\
		printf "### SublimeText ###\n# cache files for sublime text\n*.tmlanguage.cache\n">>.gitignore;\
		printf "*.tmPreferences.cache\n*.stTheme.cache\n\n# workspace files are user-specific\n">>.gitignore;\
		printf "*.sublime-workspace\n\n">>.gitignore;\
		printf "# project files should be checked into the repository, unless a significant\n">>.gitignore;\
		printf "# proportion of contributors will probably not be using SublimeText\n">>.gitignore;\
		printf "# *.sublime-project\n\n# sftp configuration file\nsftp-config.json\n\n">>.gitignore;\
		printf "# Package control specific files\nPackage Control.last-run\n">>.gitignore;\
		printf "Package Control.ca-list\nPackage Control.ca-bundle\n">>.gitignore;\
		printf "Package Control.system-ca-bundle\nPackage Control.cache/\n">>.gitignore;\
		printf "Package Control.ca-certs/\nPackage Control.merged-ca-bundle\n">>.gitignore;\
		printf "Package Control.user-ca-bundle\noscrypto-ca-bundle.crt\n">>.gitignore;\
		printf "bh_unicode_properties.cache\n\n">>.gitignore;\
		printf "# Sublime-github package stores a github token in this file\n">>.gitignore;\
		printf "# https://packagecontrol.io/packages/sublime-github\nGitHub.sublime-settings\n\n">>.gitignore;\
		printf "### TeX ###\n## Core latex/pdflatex auxiliary files:\n*.aux\n*.lof\n*.log\n">>.gitignore;\
		printf "*.lot\n*.fls\n*.toc\n*.fmt\n*.fot\n*.cb\n*.cb2\n\n## Intermediate documents:\n">>.gitignore;\
		printf "*.dvi\n*.xdv\n*-converted-to.*\n">>.gitignore;\
		printf "# these rules might exclude image files for figures etc.\n# *.ps\n# *.eps\n">>.gitignore;\
		printf "# *.pdf\n\n">>.gitignore;\
		printf "## Generated if empty string is given at \"Please type another file name for output:\"\n">>.gitignore;\
		printf ".pdf\n\n">>.gitignore;\
		printf "## Bibliography auxiliary files (bibtex/biblatex/biber):\n*.bbl\n*.bcf\n*.blg\n">>.gitignore;\
		printf "*-blx.aux\n*-blx.bib\n*.run.xml\n\n">>.gitignore;\
		printf "## Build tool auxiliary files:\n*.fdb_latexmk\n*.synctex\n*.synctex(busy)\n">>.gitignore;\
		printf "*.synctex.gz\n*.synctex.gz(busy)\n*.pdfsync\n*Notes.bib\n\n">>.gitignore;\
		printf "## Auxiliary and intermediate files from other packages:\n# algorithms\n">>.gitignore;\
		printf "*.alg\n*.loa\n\n# achemso\nacs-*.bib\n\n# amsthm\n*.thm\n\n# beamer\n*.nav\n">>.gitignore;\
		printf "*.pre\n*.snm\n*.vrb\n\n# changes\n*.soc\n\n# cprotect\n*.cpt\n\n">>.gitignore;\
		printf "# elsarticle (documentclass of Elsevier journals)\n*.spl\n\n# endnotes\n">>.gitignore;\
		printf "*.ent\n\n# fixme\n*.lox\n\n# feynmf/feynmp\n*.mf\n*.mp\n*.t[1-9]\n*.t[1-9][0-9]\n">>.gitignore;\
		printf "*.tfm\n\n#(r)(e)ledmac/(r)(e)ledpar\n*.end\n*.?end\n*.[1-9]\n*.[1-9][0-9]\n">>.gitignore;\
		printf "*.[1-9][0-9][0-9]\n*.[1-9]R\n*.[1-9][0-9]R\n*.[1-9][0-9][0-9]R\n*.eledsec[1-9]\n">>.gitignore;\
		printf "*.eledsec[1-9]R\n*.eledsec[1-9][0-9]\n*.eledsec[1-9][0-9]R\n">>.gitignore;\
		printf "*.eledsec[1-9][0-9][0-9]\n*.eledsec[1-9][0-9][0-9]R\n\n# glossaries\n*.acn\n.acr\n">>.gitignore;\
		printf "*.glg\n*.glo\n*.gls\n*.glsdefs\n\n# gnuplottex\n*-gnuplottex-*\n\n# gregoriotex\n">>.gitignore;\
		printf "*.gaux\n*.gtex\n\n# hyperref\n*.brf\n\n# knitr\n*-concordance.tex\n">>.gitignore;\
		printf "# TODO Comment the next line if you want to keep your tikz graphics files\n*.tikz\n">>.gitignore;\
		printf "*-tikzDictionary\n\n# listings\n*.lol\n\n# makeidx\n*.idx\n*.ilg\n*.ind\n*.ist\n\n">>.gitignore;\
		printf "# minitoc\n*.maf\n*.mlf\n*.mlt\n*.mtc[0-9]*\n*.slf[0-9]*\n*.slt[0-9]*\n*.stc[0-9]*\n\n">>.gitignore;\
		printf "# minted\n_minted*\n*.pyg\n\n# morewrites\n*.mw\n\n# nomencl\n*.nlo\n\n# pax\n">>.gitignore;\
		printf "*.pax\n\n# pdfpcnotes\n*.pdfpc\n\n# sagetex\n*.sagetex.sage\n*.sagetex.py\n">>.gitignore;\
		printf "*.sagetex.scmd\n\n# scrwfile\n*.wrt\n\n# sympy\n*.sout\n*.sympy\n">>.gitignore;\
		printf "sympy-plots-for-*.tex/\n\n# pdfcomment\n*.upa\n*.upb\n\n# pythontex\n*.pytxcode\n">>.gitignore;\
		printf "pythontex-files-*/\n\n# thmtools\n*.loe\n\n# TikZ & PGF\n*.dpth\n*.md5\n*.auxlock\n\n">>.gitignore;\
		printf "# todonotes\n*.tdo\n\n# easy-todo\n*.lod\n\n# xindy\n*.xdy\n\n">>.gitignore;\
		printf "# xypic precompiled matrices\n*.xyc\n\n# endfloat\n*.ttt\n*.fff\n\n# Latexian\n">>.gitignore;\
		printf "TSWLatexianTemp*\n\n## Editors:\n# WinEdt\n*.sav\n\n# Texpad\n.texpadtmp\n\n">>.gitignore;\
		printf "# Kile\n*.backup\n\n# KBibTeX\n*~[0-9]*\n\n# auto folder when using emacs and auctex\n">>.gitignore;\
		printf "/auto/*\n\n# expex forward references with \gathertags\n*-tags.tex\n\n">>.gitignore;\
		printf "### TeX Patch ###\n# glossaries\n*.glstex\n\n">>.gitignore;\
		printf "### Vim ###\n# swap\n.sw[a-p]\n.*.sw[a-p]\n# session\nSession.vim\n# temporary\n">>.gitignore;\
		printf ".netrwhist\n# auto-generated tag files\ntags\n\n">>.gitignore;\
		printf "### Windows ###\n# Windows thumbnail cache files\nThumbs.db\nehthumbs.db\n">>.gitignore;\
		printf "ehthumbs_vista.db\n\n# Folder config file\nDesktop.ini\n\n">>.gitignore;\
		printf "# Recycle Bin used on file shares\n\$RECYCLE.BIN/\n\n# Windows Installer files\n">>.gitignore;\
		printf "*.cab\n*.msi\n*.msm\n*.msp\n\n# Windows shortcuts\n*.lnk\n\n\n">>.gitignore;\
		printf "# End of https://www.gitignore.io/api/c,cuda,osx,git,c++,vim,emacs,eclipse,tex,windows,fortran,sublimetext,matlab\n">>.gitignore;\
	fi
#=============================.gitignore RAW TEXT=============================#
