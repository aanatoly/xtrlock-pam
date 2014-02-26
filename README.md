# xtrlock-pam

**xtrlock** is PAM based X11 screen locker that hides all windows

## Features 

 - PAM based
   - use any PAM module you like
   - run by regular user, no root setuid is ever required
 - Background actions
   - **blank** blank the screen, hide desktop content
   - **none** do not hide anything, as former xtrlock did
   - **bg** hide windows, show just root window background. This work if root
     window has one. Run `xprop -root | grep PMAP` to check
     
## Usage
Run **xtrlock** to lock, and type your password to unlock.

Default behaviour
```bash
xtrlock_pam
xtrlock_pam -p system-local-login -b blank
```

Another run
```bash
xtrlock_pam -p login -b none
```

Try this!
```bash
xtrlock_pam -b bg
```
