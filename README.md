# xtrlock-pam

**xtrlock** is PAM based X11 screen locker that hides all windows

## Features 

 - PAM based
   - use any PAM module you like
   - run by regular user, no root setuid is ever required
 - Background actions
   - **blank** blanks the screen, hides desktop content
   - **none** do nothing, as former xtrlock did

## Usage
Default behaviour
```bash
xtrlock_pam
xtrlock_pam -p system-local-login -b blank
```

Another run
```bash
xtrlock_pam -p login -b none
```
