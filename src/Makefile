## miniconf makefiles ## 1.1 ##

TOPDIR := ..

xtrlock_pam_src = xtrlock.c auth_pam.c
xtrlock_pam_cflags += $(X11_CFLAGS)
xtrlock_pam_libs += $(X11_LIBS) -lpam
xtrlock_pam_type = bin

include $(TOPDIR)/.config/rules.mk
