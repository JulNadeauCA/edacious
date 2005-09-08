#!/bin/sh
#
#	$Csoft: release.sh,v 1.4 2005/01/16 10:18:47 vedge Exp $

VER=`date +%m%d%Y`
PROJ=agar-eda
PROJCVS=Agar-EDA
PROJNAME="Agar-EDA"
PROJSTATE=beta

DISTFILE=${PROJ}-${VER}
CVSROOT=/home/cvs/CVSROOT

cd ..
echo "snapshot: ${PROJ}-${VER}"
rm -fr ${PROJ}-${VER}
cp -fRp ${PROJ} ${PROJ}-${VER}

(cd ${CVSROOT} &&
 mv -f ${PROJCVS}-ChangeLog ${PROJCVS}-ChangeLog-${VER} &&
 touch ${PROJCVS}-ChangeLog &&
 chgrp csoft ${PROJCVS}-ChangeLog &&
 chmod 666 ${PROJCVS}-ChangeLog)

cp -f ${CVSROOT}/${PROJCVS}-ChangeLog-${VER} ${PROJ}-${VER}/ChangeLog-${VER}
cp -f ${CVSROOT}/${PROJCVS}-ChangeLog-${VER} ${PROJ}-${VER}.ChangeLog

rm -fR `find ${PROJ}-${VER} \( -name CVS \
    -or -name \*~ \
    -or -name \*.o \
    -or -name \*.a \
    -or -name \*.core \
    -or -name .\*.swp \
    -or -name .depend \
    -or -name .xvpics \)`

echo "packaging"
tar -f ${DISTFILE}.tar -c ${PROJ}-${VER}
gzip -f9 ${DISTFILE}.tar
md5 ${DISTFILE}.tar.gz > ${DISTFILE}.tar.gz.md5
rmd160 ${DISTFILE}.tar.gz >> ${DISTFILE}.tar.gz.md5
sha1 ${DISTFILE}.tar.gz >> ${DISTFILE}.tar.gz.md5
gpg -ab ${DISTFILE}.tar.gz

echo "uploading"
scp -C ${DISTFILE}.{tar.gz,tar.gz.md5,tar.gz.asc,ChangeLog} vedge@resin:www/snap
ssh vedge@resin "cp -f www/snap/${DISTFILE}.{tar.gz,tar.gz.md5,tar.gz.asc,ChangeLog} www/beta.csoft.org/${PROJ} && ls -l www/beta.csoft.org/${PROJ}/${DISTFILE}.*"

echo "notifying agar-announce@"
TMP=`mktemp /tmp/agarannounceXXXXXXXX`
cat > $TMP << EOF
From: Wilbern Cobb <vedge@csoft.org>
To: agar-announce@lists.csoft.net
Subject: New ${PROJNAME} ${PROJSTATE}: $VER
X-Mailer: announce.sh
X-PGP-Key: 206C63E6

A new ${PROJNAME} release has been uploaded to beta.csoft.org.
This is a beta release so please help by testing it thoroughly and
letting me know of any problems you might run into.

	http://beta.csoft.org/$PROJ/$PROJ-$VER.tar.gz
	http://beta.csoft.org/$PROJ/$PROJ-$VER.tar.gz.asc
	http://beta.csoft.org/$PROJ/$PROJ-$VER.tar.gz.md5
	http://beta.csoft.org/$PROJ/$PROJ-$VER.ChangeLog

Below is the summary of changes since the last release. If you wish to
receive individual e-mails whenever commits are made, send an empty
e-mail to <source-diff-subscribe@lists.csoft.net> (unified diffs inline),
or <source-changes-subscribe@lists.csoft.net> (no diffs).

EOF
cat ${PROJ}-${VER}.ChangeLog >> $TMP
cat $TMP | sendmail -t
rm -f $TMP
