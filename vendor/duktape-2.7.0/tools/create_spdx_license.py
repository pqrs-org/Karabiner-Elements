#!/usr/bin/env python2
#
#  Helper to create an SPDX license file (http://spdx.org)
#
#  This must be executed when the dist/ directory is otherwise complete,
#  except for the SPDX license, so that the file lists and such contained
#  in the SPDX license will be correct.
#
#  The utility outputs RDF/XML to specified file:
#
#    $ python create_spdx_license.py /tmp/license.spdx
#
#  Then, validate with SPDXViewer and SPDXTools:
#
#    $ java -jar SPDXViewer.jar /tmp/license.spdx
#    $ java -jar java -jar spdx-tools-1.2.5-jar-with-dependencies.jar RdfToHtml /tmp/license.spdx /tmp/license.html
#
#  Finally, copy to dist:
#
#    $ cp /tmp/license.spdx dist/license.spdx
#
#  SPDX FAQ indicates there is no standard extension for an SPDX license file
#  but '.spdx' is a common practice.
#
#  The algorithm to compute a "verification code", implemented in this file,
#  can be verified as follows:
#
#    # build dist tar.xz, copy to /tmp/duktape-N.N.N.tar.xz
#    $ cd /tmp
#    $ tar xvfJ duktape-N.N.N.tar.xz
#    $ rm duktape-N.N.N/license.spdx  # remove file excluded from verification code
#    $ java -jar spdx-tools-1.2.5-jar-with-dependencies.jar GenerateVerificationCode /tmp/duktape-N.N.N/
#
#  Compare the resulting verification code manually with the one in license.spdx.
#
#  Resources:
#
#   - http://spdx.org/about-spdx/faqs
#   - http://wiki.spdx.org/view/Technical_Team/Best_Practices
#

import os
import sys
import re
import datetime
import sha
import rdflib
from rdflib import URIRef, BNode, Literal, Namespace

RDF = Namespace('http://www.w3.org/1999/02/22-rdf-syntax-ns#')
RDFS = Namespace('http://www.w3.org/2000/01/rdf-schema#')
XSD = Namespace('http://www.w3.org/2001/XMLSchema#')
SPDX = Namespace('http://spdx.org/rdf/terms#')
DOAP = Namespace('http://usefulinc.com/ns/doap#')
DUKTAPE = Namespace('http://duktape.org/rdf/terms#')

def checksumFile(g, filename):
    f = open(filename, 'rb')
    d = f.read()
    f.close()
    shasum = sha.sha(d).digest().encode('hex').lower()

    csum_node = BNode()
    g.add((csum_node, RDF.type, SPDX.Checksum))
    g.add((csum_node, SPDX.algorithm, SPDX.checksumAlgorithm_sha1))
    g.add((csum_node, SPDX.checksumValue, Literal(shasum)))

    return csum_node

def computePackageVerification(g, dirname, excluded):
    # SPDX 1.2 Section 4.7
    # The SPDXTools command "GenerateVerificationCode" can be used to
    # check the verification codes created.  Note that you must manually
    # remove "license.spdx" from the unpacked dist directory before
    # computing the verification code.

    verify_node = BNode()

    hashes = []
    for dirpath, dirnames, filenames in os.walk(dirname):
        for fn in filenames:
            full_fn = os.path.join(dirpath, fn)
            f = open(full_fn, 'rb')
            d = f.read()
            f.close()

            if full_fn in excluded:
                #print('excluded in verification: ' + full_fn)
                continue
            #print('included in verification: ' + full_fn)

            file_sha1 = sha.sha(d).digest().encode('hex').lower()
            hashes.append(file_sha1)

    #print(repr(hashes))
    hashes.sort()
    #print(repr(hashes))
    verify_code = sha.sha(''.join(hashes)).digest().encode('hex').lower()

    for fn in excluded:
        g.add((verify_node, SPDX.packageVerificationCodeExcludedFile, Literal(fn)))
    g.add((verify_node, SPDX.packageVerificationCodeValue, Literal(verify_code)))

    return verify_node

def fileType(filename):
    ign, ext = os.path.splitext(filename)
    if ext in [ '.c', '.h', '.js' ]:
        return SPDX.fileType_source
    else:
        return SPDX.fileType_other

def getDuktapeVersion():
    f = open('./src/duktape.h')
    re_ver = re.compile(r'^#define\s+DUK_VERSION\s+(\d+)L$')
    for line in f:
        line = line.strip()
        m = re_ver.match(line)
        if m is None:
            continue
        ver = int(m.group(1))
        return '%d.%d.%d' % ((ver / 10000) % 100,
                             (ver / 100) % 100,
                             ver % 100)

    raise Exception('could not figure out Duktape version')

def main():
    outfile = sys.argv[1]

    if not os.path.exists('CONTRIBUTING.md') and os.path.exists('tests/ecmascript'):
        sys.stderr.write('Invalid CWD, must be in Duktape root with dist/ built')
        sys.exit(1)
    os.chdir('dist')
    if not os.path.exists('Makefile.cmdline'):
        sys.stderr.write('Invalid CWD, must be in Duktape root with dist/ built')
        sys.exit(1)

    duktape_version = getDuktapeVersion()
    duktape_pkgname = 'duktape-' + duktape_version + '.tar.xz'
    now = datetime.datetime.utcnow()
    now = datetime.datetime(now.year, now.month, now.day, now.hour, now.minute, now.second)
    creation_date = Literal(now.isoformat() + 'Z', datatype=XSD.dateTime)
    duktape_org = Literal('Organization: duktape.org')
    mit_license = URIRef('http://spdx.org/licenses/MIT')
    duktape_copyright = Literal('Copyright 2013-2017 Duktape authors (see AUTHORS.rst in the Duktape distributable)')

    g = rdflib.Graph()

    crea_node = BNode()
    g.add((crea_node, RDF.type, SPDX.CreationInfo))
    g.add((crea_node, RDFS.comment, Literal('')))
    g.add((crea_node, SPDX.creator, duktape_org))
    g.add((crea_node, SPDX.created, creation_date))
    g.add((crea_node, SPDX.licenseListVersion, Literal('1.20')))  # http://spdx.org/licenses/

    # 'name' should not include a version number (see best practices)
    pkg_node = BNode()
    g.add((pkg_node, RDF.type, SPDX.Package))
    g.add((pkg_node, SPDX.name, Literal('Duktape')))
    g.add((pkg_node, SPDX.versionInfo, Literal(duktape_version)))
    g.add((pkg_node, SPDX.packageFileName, Literal(duktape_pkgname)))
    g.add((pkg_node, SPDX.supplier, duktape_org))
    g.add((pkg_node, SPDX.originator, duktape_org))
    g.add((pkg_node, SPDX.downloadLocation, Literal('http://duktape.org/' + duktape_pkgname, datatype=XSD.anyURI)))
    g.add((pkg_node, SPDX.homePage, Literal('http://duktape.org/', datatype=XSD.anyURI)))
    verify_node = computePackageVerification(g, '.', [ './license.spdx' ])
    g.add((pkg_node, SPDX.packageVerificationCode, verify_node))
    # SPDX.checksum: omitted because license is inside the package
    g.add((pkg_node, SPDX.sourceInfo, Literal('Official duktape.org release built from GitHub repo https://github.com/svaarala/duktape.')))

    # NOTE: MIT license alone is sufficient for now, because Duktape, Lua,
    # Murmurhash2, and CommonJS (though probably not even relevant for
    # licensing) are all MIT.
    g.add((pkg_node, SPDX.licenseConcluded, mit_license))
    g.add((pkg_node, SPDX.licenseInfoFromFiles, mit_license))
    g.add((pkg_node, SPDX.licenseDeclared, mit_license))
    g.add((pkg_node, SPDX.licenseComments, Literal('Duktape is copyrighted by its authors and licensed under the MIT license.  MurmurHash2 is used internally, it is also under the MIT license. Duktape module loader is based on the CommonJS module loading specification (without sharing any code), CommonJS is under the MIT license.')))
    g.add((pkg_node, SPDX.copyrightText, duktape_copyright))
    g.add((pkg_node, SPDX.summary, Literal('Duktape ECMAScript interpreter')))
    g.add((pkg_node, SPDX.description, Literal('Duktape is an embeddable Javascript engine, with a focus on portability and compact footprint')))
    # hasFile properties added separately below

    #reviewed_node = BNode()
    #g.add((reviewed_node, RDF.type, SPDX.Review))
    #g.add((reviewed_node, SPDX.reviewer, XXX))
    #g.add((reviewed_node, SPDX.reviewDate, XXX))
    #g.add((reviewed_node, RDFS.comment, ''))

    spdx_doc = BNode()
    g.add((spdx_doc, RDF.type, SPDX.SpdxDocument))
    g.add((spdx_doc, SPDX.specVersion, Literal('SPDX-1.2')))
    g.add((spdx_doc, SPDX.dataLicense, URIRef('http://spdx.org/licenses/CC0-1.0')))
    g.add((spdx_doc, RDFS.comment, Literal('SPDX license for Duktape ' + duktape_version)))
    g.add((spdx_doc, SPDX.creationInfo, crea_node))
    g.add((spdx_doc, SPDX.describesPackage, pkg_node))
    # SPDX.hasExtractedLicensingInfo
    # SPDX.reviewed
    # SPDX.referencesFile: added below

    for dirpath, dirnames, filenames in os.walk('.'):
        for fn in filenames:
            full_fn = os.path.join(dirpath, fn)
            #print('# file: ' + full_fn)

            file_node = BNode()
            g.add((file_node, RDF.type, SPDX.File))
            g.add((file_node, SPDX.fileName, Literal(full_fn)))
            g.add((file_node, SPDX.fileType, fileType(full_fn)))
            g.add((file_node, SPDX.checksum, checksumFile(g, full_fn)))

            # Here we assume that LICENSE.txt provides the actual "in file"
            # licensing information, and everything else is implicitly under
            # MIT license.
            g.add((file_node, SPDX.licenseConcluded, mit_license))
            if full_fn == './LICENSE.txt':
                g.add((file_node, SPDX.licenseInfoInFile, mit_license))
            else:
                g.add((file_node, SPDX.licenseInfoInFile, URIRef(SPDX.none)))

            # SPDX.licenseComments
            g.add((file_node, SPDX.copyrightText, duktape_copyright))
            # SPDX.noticeText
            # SPDX.artifactOf
            # SPDX.fileDependency
            # SPDX.fileContributor

            # XXX: should referencesFile include all files?
            g.add((spdx_doc, SPDX.referencesFile, file_node))

            g.add((pkg_node, SPDX.hasFile, file_node))

    # Serialize into RDF/XML directly.  We could also serialize into
    # N-Triples and use external tools (like 'rapper') to get cleaner,
    # abbreviated output.

    #print('# Duktape SPDX license file (autogenerated)')
    #print(g.serialize(format='turtle'))
    #print(g.serialize(format='nt'))
    f = open(outfile, 'wb')
    #f.write(g.serialize(format='rdf/xml'))
    f.write(g.serialize(format='xml'))
    f.close()

if __name__ == '__main__':
    main()
