#!/bin/bash

crtools="../../../../crtools"

cleanup_class() {
	rm -f ./*.class
}

javac HelloWorld.java || exit 1

set -x

rm -rf dump
mkdir dump

setsid java HelloWorld &

pid=${!}

echo Lanuched java application with pid $pid in background

${crtools} dump -D dump -o dump.log -v 4  -t ${pid} || {
	echo "Dump failed"
	exit 1
}

echo "Dumped, restoring and waiting for completion"

${crtools} restore -D dump -o restore.log -v 4 -t ${pid} || {
	echo "Restore failed"
	exit 1
}

echo PASS
