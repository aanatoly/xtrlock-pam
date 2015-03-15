# xtrlock-pam

**xtrlock-pam** is PAM based X11 screen locker that hides desktop content.

<a href="http://aanatoly.github.io/xtrlock-pam/images/normal.png">
<img src="http://aanatoly.github.io/xtrlock-pam/images/normal-th.png"/></a>
<a href="http://aanatoly.github.io/xtrlock-pam/images/locked.png">
<img src="http://aanatoly.github.io/xtrlock-pam/images/locked-th.png"></a>

## Features 

 - PAM based
   - use any PAM module you like
   - run by regular user, no root setuid is ever required
 - Background actions
   - **blank** blank the screen, hide desktop content
   - **none** do not hide anything, as former xtrlock did
   - **bg** hide windows, show just root window background. This work if root
     window has one.
     
## Usage
Run **xtrlock-pam** to lock, and type your password to unlock.


```text
$ xtrlock-pam -h
xtrlock-pam 3.3 - PAM based X11 screen locker
Usage: xtrlock-pam [options...]
Options:
 -h      This help message
 -p MOD  PAM module, default is 'system-local-login'
 -b BG   background action, none, blank or bg, default is bg

```

Default is
```
$ xtrlock-pam
$ xtrlock-pam -p system-local-login -b bg
```

## Installation

```bash
./configure
make
# system install - as root
make install
# personal install - as user
cp src/xtrlock-pam ~/bin
```

## More
Project page http://aanatoly.github.io/xtrlock-pam
