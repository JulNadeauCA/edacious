#!/bin/sh

VER=`grep "HDEFINE(VERSION" configure.in |awk -F\\" '{print $3}' |awk -F\\\ '{print $1}'`
PHASE=beta
PROJNAME=Edacious
PROJECT=edacious
DISTFILE=${PROJECT}-${VER}
HOST=resin.csoft.net
RUSER=vedge
MAILER="sendmail -t"

echo "Packaging ${DISTFILE}"

cd ..
rm -fr ${DISTFILE}
cp -fRp ${PROJECT} ${DISTFILE}

rm -fR `find ${DISTFILE} \( -name .svn \
    -or -name \*~ \
    -or -name \*.o \
    -or -name \*.a \
    -or -name \*.core \
    -or -name .\*.swp \
    -or -name .depend \
    -or -name .xvpics \)`

tar -f ${DISTFILE}.tar -c ${DISTFILE}
gzip -f9 ${DISTFILE}.tar
openssl md5 ${DISTFILE}.tar.gz > ${DISTFILE}.tar.gz.md5
openssl rmd160 ${DISTFILE}.tar.gz >> ${DISTFILE}.tar.gz.md5
openssl sha1 ${DISTFILE}.tar.gz >> ${DISTFILE}.tar.gz.md5
gpg -ab ${DISTFILE}.tar.gz

if [ "$1" = "commit" ]; then
	echo "uploading"
	scp -C ${DISTFILE}.{tar.gz,tar.gz.md5,tar.gz.asc} ${RUSER}@${HOST}:www/$PHASE.hypertriton.com/${PROJECT}

	echo "notifying eda-announce@"
	TMP=`mktemp /tmp/agarannounceXXXXXXXX`
	cat > $TMP << EOF
From: Julien Nadeau <vedge@hypertriton.com>
To: eda-announce@hypertriton.com
Subject: New Edacious release: $VER
X-Mailer: announce.sh
X-PGP-Key: 206C63E6

A new $PROJNAME release has been uploaded to $PHASE.hypertriton.com.

	http://$PHASE.hypertriton.com/$PROJECT/$DISTFILE.tar.gz
	http://$PHASE.hypertriton.com/$PROJECT/$DISTFILE.tar.gz.asc
	http://$PHASE.hypertriton.com/$PROJECT/$DISTFILE.tar.gz.md5

EOF
	cat $TMP | ${MAILER}
	rm -f $TMP
fi
