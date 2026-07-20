ifneq '' '$(findstring clang++,$(CXX))'
  CLANG_MAJOR=$(shell echo | $(CXX) -dM -E - | grep __clang_major__ | cut -d' ' -f3)
  ifeq ($(shell expr $(CLANG_MAJOR) \>= 22), 1)
    # __COUNTER__ is all over glib header files.
    # Arguably reasoable to complain about from -pedantic, but it's not like i could just fix that.
    export CXXFLAGS += -Wno-c2y-extensions
  endif
endif
