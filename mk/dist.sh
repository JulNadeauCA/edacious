#!/bin/sh
#
#	$Csoft: dist.sh,v 1.17 2004/03/13 09:26:44 vedge Exp $

VER=`grep "HDEFINE(VERSION" configure.in |awk -F\\" '{print $3}' |awk -F\\\ '{print $1}'`
PHASE=beta
PROJNAME=Agar-EDA
PROJECT=agar-eda
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
	scp -C ${DISTFILE}.{tar.gz,tar.gz.md5,tar.gz.asc} ${RUSER}@${HOST}:www/$PHASE.csoft.org/${PROJECT}

	echo "notifying agar-announce@"
	TMP=`mktemp /tmp/agarannounceXXXXXXXX`
	cat > $TMP << EOF
From: Wilbern Cobb <vedge@csoft.org>
To: agar-announce@lists.csoft.net
Subject: New Agar release: $VER
X-Mailer: announce.sh
X-PGP-Key: 206C63E6

A new $PROJNAME release has been uploaded to $PHASE.csoft.org.

	http://$PHASE.csoft.org/$PROJECT/$DISTFILE.tar.gz
	http://$PHASE.csoft.org/$PROJECT/$DISTFILE.tar.gz.asc
	http://$PHASE.csoft.org/$PROJECT/$DISTFILE.tar.gz.md5

EOF
	cat $TMP | ${MAILER}
	rm -f $TMP
fi
