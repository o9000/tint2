#!/bin/sh

VERSION=0.13.3

echo '#define VERSION_STRING "'$VERSION'"' > version.h
echo $VERSION
