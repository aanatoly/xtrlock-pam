## miniconf makefiles ## 1.1 ##

TOPDIR := ..

xtrlock-pam_src = xtrlock.c auth_pam.c
xtrlock-pam_cflags += $(X11_CFLAGS)
xtrlock-pam_libs += $(X11_LIBS) -lpam
xtrlock-pam_type = bin

include $(TOPDIR)/.config/rules.mk
