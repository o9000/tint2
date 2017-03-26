#!/bin/sh

MAJOR=0.14
DIRTY=""

VERSION=0.14

echo '#define VERSION_STRING "'$VERSION'"' > version.h
echo $VERSION
