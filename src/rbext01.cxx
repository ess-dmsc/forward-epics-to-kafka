#include <cstdlib>
#include <cstdio>
#include <ruby.h>
#include <logger.h>
#include <rbext01.h>

#define RUBY_LINKAGE extern "C"

static std::atomic_int ai1_ {0};

std::atomic_int & ai1() { return ai1_; }


RUBY_LINKAGE VALUE kl01_update(VALUE self) {
	LOG(3, "update..");
	ai1() += 1;
	return Qnil;
}

RUBY_LINKAGE void Init_librbext01() {
	printf("init rbext01\n");
	using RUBY_FUNC_TYPE = VALUE(*)(...);
	VALUE kl01 = rb_define_class("RBEXT01", rb_cObject);
	rb_define_method(kl01, "update", (RUBY_FUNC_TYPE)kl01_update, 0);
}
