SURFFS (web surfing file system)
===============

SURFFS is  linux file system representing html pages and links from some http server as file tree.

It creates read-only folder structure where each directory corresponds to one html page. Each directory contains following:
- file page.html - html text of page
- file url - url of page (without protocol)
- file status - "ok" / "error" depending on http loading status
- file loading.log - some information about loading html page. This file contains error messages in case of http loading failed. It also contains description of each html link: whether it was added or skipped
- subdirectory for each html < a > element. Name of directory generated from link title (ascii control symbols are replaced with spaces. All data in <> brackets within title ignored). If directory points to page which already been pointed from another directory, symlink to another directory will be created.

Surffs was developed just for fun and self-education purpose, so it has some limitaions:
- only http protocol supported (no https)
- surffs does not resolve host name, so it requires both url and ip at mounting
- links to another hosts not supported. I.e. if you mount surffs to www.example.com, it will be able to access web pages only from host www.example.com.
- surffs accepts only "200 OK" http response. Another codes like "301 Moved Permanently" etc. interpreted as error of loading page
- there is no memory limitation for web pages cache in surffs. Memory used for web pages will be freed only when module surffs will be removed from kernel (rmmod surffs.ko)
- if page once cannot be loaded it will be not available all time till remounting file system
- surffs does not protect multithreading operations, so simultaneous operating with several instances of surffs may cause undefined behaviour
- html links in format of < link > element or < a href="..." title="..." > are not processed

**Building:**

Surffs was built against kernel version 3.13.11-ckt32. Building against different kernel versions may require additional changes in code.
```sh
$ uname -r
3.13.11-ckt32
$ cd surffs_src_dir
$ make all
$ ls surffs.ko
surffs.ko
```
You can change log level before building: this is macro SURFFS_CURRENT_LOGLEVEL in surffs_debug.h

**Usage example:**

Example of getting data from site http://tinyeyes.com (this is pretty tiny site with a small amount of pages and links)
```sh
$ insmod surffs.ko
$ mkdir /mnt/surffs
$ surffs_mount.sh http://tinyeyes.com /mnt/surffs
$ cd /mnt/surffs/
$ ls -laR
.:
total 4
dr--r--r-- 1 root root     0 Mar 29 15:17 .
drwxr-xr-x 9 root root  4096 Mar 29 15:14 ..
dr--r--r-- 1 root root     0 Mar 29 15:17 About us |_link3
dr--r--r-- 1 root root     0 Mar 29 15:17 Contact us |_link4
dr--r--r-- 1 root root     0 Mar 29 15:17 Home |_link1
dr--r--r-- 1 root root     0 Mar 29 15:17 Links_link5
-r--r--r-- 1 root root   979 Mar 29 15:17 loading.log
-r--r--r-- 1 root root 13597 Mar 29 15:17 page.html
-r--r--r-- 1 root root     2 Mar 29 15:17 status
dr--r--r-- 1 root root     0 Mar 29 15:17 Terms and Conditions_link6
dr--r--r-- 1 root root     0 Mar 29 15:17 Try it! |_link2
-r--r--r-- 1 root root    13 Mar 29 15:17 url

./About us |_link3:
total 0
dr--r--r-- 1 root root    0 Mar 29 15:17 .
dr--r--r-- 1 root root    0 Mar 29 15:17 ..
lr--r--r-- 1 root root    0 Mar 29 15:17 Contact us |_link3 -> ../Contact us |_link4
lr--r--r-- 1 root root    0 Mar 29 15:17 Home |_link1 -> ../Home |_link1
lr--r--r-- 1 root root    0 Mar 29 15:17 Links_link4 -> ../Links_link5
-r--r--r-- 1 root root  696 Mar 29 15:17 loading.log
-r--r--r-- 1 root root 7649 Mar 29 15:17 page.html
-r--r--r-- 1 root root    2 Mar 29 15:17 status
lr--r--r-- 1 root root    0 Mar 29 15:17 Terms and Conditions_link5 -> ../Terms and Conditions_link6
lr--r--r-- 1 root root    0 Mar 29 15:17 Try it! |_link2 -> ../Try it! |_link2
-r--r--r-- 1 root root   20 Mar 29 15:17 url

./Contact us |_link4:
total 0
dr--r--r-- 1 root root    0 Mar 29 15:17 .
dr--r--r-- 1 root root    0 Mar 29 15:17 ..
lr--r--r-- 1 root root    0 Mar 29 15:17 About us |_link3 -> ../About us |_link3
lr--r--r-- 1 root root    0 Mar 29 15:17 Home |_link1 -> ../Home |_link1
lr--r--r-- 1 root root    0 Mar 29 15:17 Links_link4 -> ../Links_link5
-r--r--r-- 1 root root  702 Mar 29 15:17 loading.log
-r--r--r-- 1 root root 6691 Mar 29 15:17 page.html
-r--r--r-- 1 root root    2 Mar 29 15:17 status
lr--r--r-- 1 root root    0 Mar 29 15:17 Terms and Conditions_link5 -> ../Terms and Conditions_link6
lr--r--r-- 1 root root    0 Mar 29 15:17 Try it! |_link2 -> ../Try it! |_link2
-r--r--r-- 1 root root   24 Mar 29 15:17 url

./Home |_link1:
total 0
dr--r--r-- 1 root root     0 Mar 29 15:17 .
dr--r--r-- 1 root root     0 Mar 29 15:17 ..
lr--r--r-- 1 root root     0 Mar 29 15:17 About us |_link2 -> ../About us |_link3
lr--r--r-- 1 root root     0 Mar 29 15:17 Contact us |_link3 -> ../Contact us |_link4
lr--r--r-- 1 root root     0 Mar 29 15:17 Links_link4 -> ../Links_link5
-r--r--r-- 1 root root  1010 Mar 29 15:17 loading.log
-r--r--r-- 1 root root 13597 Mar 29 15:17 page.html
-r--r--r-- 1 root root     2 Mar 29 15:17 status
lr--r--r-- 1 root root     0 Mar 29 15:17 Terms and Conditions_link5 -> ../Terms and Conditions_link6
lr--r--r-- 1 root root     0 Mar 29 15:17 Try it! |_link1 -> ../Try it! |_link2
-r--r--r-- 1 root root    22 Mar 29 15:17 url

./Links_link5:
total 0
dr--r--r-- 1 root root    0 Mar 29 15:17 .
dr--r--r-- 1 root root    0 Mar 29 15:17 ..
lr--r--r-- 1 root root    0 Mar 29 15:17 About us |_link7 -> ../About us |_link3
dr--r--r-- 1 root root    0 Mar 29 15:17 babycenter.com_link2
dr--r--r-- 1 root root    0 Mar 29 15:17 BrainConnection_link4
lr--r--r-- 1 root root    0 Mar 29 15:17 Contact us |_link8 -> ../Contact us |_link4
lr--r--r-- 1 root root    0 Mar 29 15:17 Home |_link5 -> ../Home |_link1
dr--r--r-- 1 root root    0 Mar 29 15:17 IAmYourChild.org_link3
-r--r--r-- 1 root root 1916 Mar 29 15:17 loading.log
-r--r--r-- 1 root root 8387 Mar 29 15:17 page.html
dr--r--r-- 1 root root    0 Mar 29 15:17 parenting.com_link1
-r--r--r-- 1 root root    2 Mar 29 15:17 status
lr--r--r-- 1 root root    0 Mar 29 15:17 Terms and Conditions_link9 -> ../Terms and Conditions_link6
lr--r--r-- 1 root root    0 Mar 29 15:17 Try it! |_link6 -> ../Try it! |_link2
-r--r--r-- 1 root root   22 Mar 29 15:17 url

./Links_link5/babycenter.com_link2:
total 0
dr--r--r-- 1 root root   0 Mar 29 15:17 .
dr--r--r-- 1 root root   0 Mar 29 15:17 ..
-r--r--r-- 1 root root 764 Mar 29 15:17 loading.log
-r--r--r-- 1 root root   0 Mar 29 15:17 page.html
-r--r--r-- 1 root root   5 Mar 29 15:17 status
-r--r--r-- 1 root root  31 Mar 29 15:17 url

./Links_link5/BrainConnection_link4:
total 0
dr--r--r-- 1 root root   0 Mar 29 15:17 .
dr--r--r-- 1 root root   0 Mar 29 15:17 ..
-r--r--r-- 1 root root 774 Mar 29 15:17 loading.log
-r--r--r-- 1 root root   0 Mar 29 15:17 page.html
-r--r--r-- 1 root root   5 Mar 29 15:17 status
-r--r--r-- 1 root root  36 Mar 29 15:17 url

./Links_link5/IAmYourChild.org_link3:
total 0
dr--r--r-- 1 root root   0 Mar 29 15:17 .
dr--r--r-- 1 root root   0 Mar 29 15:17 ..
-r--r--r-- 1 root root 768 Mar 29 15:17 loading.log
-r--r--r-- 1 root root   0 Mar 29 15:17 page.html
-r--r--r-- 1 root root   5 Mar 29 15:17 status
-r--r--r-- 1 root root  33 Mar 29 15:17 url

./Links_link5/parenting.com_link1:
total 0
dr--r--r-- 1 root root   0 Mar 29 15:17 .
dr--r--r-- 1 root root   0 Mar 29 15:17 ..
-r--r--r-- 1 root root 762 Mar 29 15:17 loading.log
-r--r--r-- 1 root root   0 Mar 29 15:17 page.html
-r--r--r-- 1 root root   5 Mar 29 15:17 status
-r--r--r-- 1 root root  30 Mar 29 15:17 url

./Terms and Conditions_link6:
total 0
dr--r--r-- 1 root root    0 Mar 29 15:17 .
dr--r--r-- 1 root root    0 Mar 29 15:17 ..
lr--r--r-- 1 root root    0 Mar 29 15:17 back_link1 -> ../Terms and Conditions_link6/back_link3
dr--r--r-- 1 root root    0 Mar 29 15:17 back_link3
lr--r--r-- 1 root root    0 Mar 29 15:17 I agree..._link2 -> ../Terms and Conditions_link6/back_link3
-r--r--r-- 1 root root  546 Mar 29 15:17 loading.log
-r--r--r-- 1 root root 2785 Mar 29 15:17 page.html
-r--r--r-- 1 root root    2 Mar 29 15:17 status
-r--r--r-- 1 root root   27 Mar 29 15:17 url

./Terms and Conditions_link6/back_link3:
total 0
dr--r--r-- 1 root root   0 Mar 29 15:17 .
dr--r--r-- 1 root root   0 Mar 29 15:17 ..
-r--r--r-- 1 root root 778 Mar 29 15:17 loading.log
-r--r--r-- 1 root root   0 Mar 29 15:17 page.html
-r--r--r-- 1 root root   5 Mar 29 15:17 status
-r--r--r-- 1 root root  38 Mar 29 15:17 url

./Try it! |_link2:
total 0
dr--r--r-- 1 root root    0 Mar 29 15:17 .
dr--r--r-- 1 root root    0 Mar 29 15:17 ..
lr--r--r-- 1 root root    0 Mar 29 15:17 About us |_link2 -> ../About us |_link3
lr--r--r-- 1 root root    0 Mar 29 15:17 Contact us |_link3 -> ../Contact us |_link4
lr--r--r-- 1 root root    0 Mar 29 15:17 Home |_link1 -> ../Home |_link1
lr--r--r-- 1 root root    0 Mar 29 15:17 Links_link4 -> ../Links_link5
-r--r--r-- 1 root root 1131 Mar 29 15:17 loading.log
-r--r--r-- 1 root root 9555 Mar 29 15:17 page.html
-r--r--r-- 1 root root    2 Mar 29 15:17 status
lr--r--r-- 1 root root    0 Mar 29 15:17 Terms and Conditions_link5 -> ../Terms and Conditions_link6
-r--r--r-- 1 root root   23 Mar 29 15:17 url
```

Folder structure is not infinite because it bounded by one host, and all cycles resolved by symlinks, so you can use find, grep and other search utilites as usual.

### Version
0.1 beta

