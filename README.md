libnss\_command
============
Copyright (c) 2017 Jose Manuel Sanchez Madrid.
This file is licensed under MIT license. See file LICENSE for details.

## Overview
libnss\_command is a [NSS (Name Service Switch)](https://en.wikipedia.org/wiki/Name_Service_Switch) module for host names resolution. It's objective is to delegate the name resolution to an external command that can be a custom program or script. The command will be executed in order to attend a host name resolution request.

## Installation
Compile the module and copy it in the _lib_ directory:
```
make
sudo make install
```
Edit the **/etc/nsswitch.conf** file and add the **command** service to the **hosts** database. Here is an example of nsswitch.conf, where command is the last service which means that will be used to resolve names that haven't been found with other modules like the standard DNS:
```
passwd:         compat
group:          compat
shadow:         compat
gshadow:        files

hosts:          files myhostname dns command
networks:       files

protocols:      db files
services:       db files
ethers:         db files
rpc:            db files

netgroup:       nis

```
For more details on how to configure nsswitch.conf check the [GNU NSS documentation](http://www.gnu.org/software/libc/manual/html_node/NSS-Configuration-File.html#NSS-Configuration-File).

libnss\_command expects the commands to be in the following paths:
 * `/usr/local/sbin/nsscommand_gethostbyname` will be executed passing a host name as the first command argument to resolve its IP addresses.
 * `/usr/local/sbin/nsscommand_gethostbyaddr` will be executed passing an IP address as the first command argument to resolve its host name.

The commands to be executed can be changed by modifing the DEFAULT\_GETHOSTBYNAME\_COMMAND and DEFAULT\_GETHOSTBYADDR\_COMMAND constants in the nss\_command.cpp source code.

## Writing custom commands
Custom commands to manage name resolution can be written in any programming language as long as they are executable files, and they implement the following specifications:
 * nsscommand\_gethostbyname receives the host name to be resolved as the first command line argument.
 * nsscommand\_gethostbyaddr receives the ip addres to be resolved as the first command line argument.
 * The returned code by the command execution represents the result of the name resolution as it is specified by the netdb interface
    * A return code of _0_ indicates the host name was found. The host information must be written to the standard output, in the format specified later on.
    * A return code of _1_ indicates the host wasn't found, it doesn't exists.
    * A return code of _2_ indicates a temporary failure of name resolution and the client may try again.
    * A return code of _3_ indicates a no-recoverable failure of name resolution and the client should not continue trying.
    * A return code of _4_ indicates the host was found and it is valid, but there is no data for it.
 * When a host name is successfully resolved, the command must return code 0 from its execution and must provide the host data by writing it to its standard output.
    * Each line represents one field of the host entry information.
    * Each line must comply with this specific format `<type>:<data>`:
        * Each line must begin with a type
        * Following the type must be a colon ':' that separates the type from the data
        * Data must follow the colon. Leading white spaces in the data are acceptable
    * Each line can begin with one of the following types:
        * _name_. This represents the _main_ name of the host. It should be only one name field. If there are multiple name lines, they may be ignored and only one will be used.
        * _alias_. This represents an alias or alternate name of the host. There can be multiple alias lines.
        * _ip4_. This represents an IPv4 address of the host in decimal dot notation. There can be multiple ip4 lines.

An example of a valid command output for a found host:
```
name: gateway.mycompany.com
alias: gateway.local.
alias: gw
alias: gateway
ip4: 192.168.0.1
ip4: 192.168.0.2
```
Sample scripts can be found in the resources directory.

