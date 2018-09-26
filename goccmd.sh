#!/bin/bash
if [ 0"$GOC" = "0" ]; then
    echo "\$GOC not found, please set \$GOC to GOCint root folder"
else 
    echo $GOC
fi
if [ $# != 1 ] ; then 
    echo "USAGE: $0 args" 
    echo " e.g.: $0 ./goccmd.sh wb" 
    exit 1; 
fi 

cd $GOC/tutorials/bios-boot-tutorial/
./bios-boot-tutorial.py -$1


