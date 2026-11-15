/*
 * Stellarium
 * Copyright (C) 2026 Ruslan Kabatsayev
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#include "testRemoteControl.hpp"
#include <QDir>
#include <QFile>
#include <QScopedPointer>
#include <QTcpServer>

#include "qtwebapp/httpserver/httprequest.h"
#include "qtwebapp/httpserver/httpresponse.h"
#include "qtwebapp/httpserver/staticfilecontroller.h"

QTEST_GUILESS_MAIN(TestRemoteControl)

void TestRemoteControl::initTestCase()
{
    QVERIFY(tempDir.isValid());
    docroot = tempDir.path() + "/webroot";
    QVERIFY(QDir().mkpath(docroot + "/sub"));

    const auto writeFile = [](const QString& fileName, const QByteArray& contents)
    {
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly))
            return false;
        return file.write(contents) == contents.size();
    };
    QVERIFY(writeFile(docroot + "/inside.txt", "inside"));
    QVERIFY(writeFile(docroot + "/sub/inside.txt", "nested"));
    QVERIFY(writeFile(docroot + "/..notes", "notes"));
    QVERIFY(writeFile(tempDir.path() + "/secret.txt", "secret"));
    QVERIFY(writeFile(tempDir.path() + "/webrootsecret.txt", "sibling"));
}

void TestRemoteControl::testPaths_data()
{
    QTest::addColumn<QByteArray>("url");
    QTest::addColumn<int>("expectedStatus");

    QTest::newRow("normal file") << QByteArray("/inside.txt") << 200;
    QTest::newRow("dot-dot filename") << QByteArray("/..notes") << 200;
#ifdef Q_OS_WIN
    QTest::newRow("encoded backslash inside root") << QByteArray("/sub%5Cinside.txt") << 200;
    QTest::newRow("encoded backslash traversal") << QByteArray("/%5C..%5Csecret.txt") << 403;
    QTest::newRow("unencoded backslash traversal") << QByteArray("/\\..\\secret.txt") << 403;
    QTest::newRow("non-immediate encoded backslash traversal") << QByteArray("/sub%5C..%5C..%5Csecret.txt") << 403;
#endif
    QTest::newRow("non-immediate slash traversal") << QByteArray("/sub/../../secret.txt") << 403;
    QTest::newRow("slash traversal") << QByteArray("/../secret.txt") << 403;
    QTest::newRow("bare dot-dot") << QByteArray("..") << 403;
    QTest::newRow("relative sibling path") << QByteArray("secret.txt") << 403;
}

void TestRemoteControl::testPaths()
{
    QFETCH(QByteArray, url);
    QFETCH(int, expectedStatus);

    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    QTcpSocket client;
    client.connectToHost(QHostAddress::LocalHost, server.serverPort());
    QVERIFY(client.waitForConnected());
    QVERIFY(server.waitForNewConnection(2000));
    QScopedPointer<QTcpSocket> peer(server.nextPendingConnection());
    QVERIFY(peer);

    const QByteArray rawRequest = "GET " + url + " HTTP/1.1\r\nHost: localhost\r\n\r\n";
    QCOMPARE(client.write(rawRequest), qint64(rawRequest.size()));
    QVERIFY(client.waitForBytesWritten());
    QVERIFY(peer->waitForReadyRead());

    HttpRequest request(4096, 4096);
    while (request.getStatus() != HttpRequest::complete && request.getStatus() != HttpRequest::abort)
    {
        if (!peer->canReadLine())
            QVERIFY(peer->waitForReadyRead());
        request.readFromSocket(peer.data());
    }
    QCOMPARE(request.getStatus(), HttpRequest::complete);

    StaticFileControllerSettings settings;
    settings.path = docroot;
    StaticFileController controller(settings);
    HttpResponse response(peer.data());
    controller.service(request, response);
    QCOMPARE(response.getStatusCode(), expectedStatus);
}

void TestRemoteControl::testMultipartFile_data()
{
    QTest::addColumn<QByteArray>("fileData");
    QTest::addColumn<QByteArray>("expectedContents");

    QTest::newRow("nonempty file") << QByteArray("contents\r\n") << QByteArray("contents");
    QTest::newRow("CRLF file contents") << QByteArray("\r\n\r\n") << QByteArray("\r\n");
    QTest::newRow("empty with body separator") << QByteArray("\r\n") << QByteArray();
    QTest::newRow("empty without body separator") << QByteArray() << QByteArray();
}

void TestRemoteControl::testMultipartFile()
{
    QFETCH(QByteArray, fileData);
    QFETCH(QByteArray, expectedContents);

    const QByteArray body = "--boundary\r\n"
                            "Content-Disposition: form-data; name=\"file\"; filename=\"upload.txt\"\r\n"
                            "\r\n" + fileData + "--boundary--\r\n";
    const QByteArray rawRequest = "POST /upload HTTP/1.1\r\n"
                                  "Host: localhost\r\n"
                                  "Content-Type: multipart/form-data; boundary=boundary\r\n"
                                  "Content-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body;

    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    QTcpSocket client;
    client.connectToHost(QHostAddress::LocalHost, server.serverPort());
    QVERIFY(client.waitForConnected());
    QVERIFY(server.waitForNewConnection(2000));
    QScopedPointer<QTcpSocket> peer(server.nextPendingConnection());
    QVERIFY(peer);

    QCOMPARE(client.write(rawRequest), qint64(rawRequest.size()));
    QVERIFY(client.waitForBytesWritten());
    HttpRequest request(4096, 4096);
    while (request.getStatus() != HttpRequest::complete && request.getStatus() != HttpRequest::abort)
    {
        if (!peer->bytesAvailable())
            QVERIFY(peer->waitForReadyRead());
        request.readFromSocket(peer.data());
    }

    QCOMPARE(request.getStatus(), HttpRequest::complete);
    QTemporaryFile* uploadedFile = request.getUploadedFile("file");
    QVERIFY(uploadedFile);
    QVERIFY(uploadedFile->isOpen());
    QCOMPARE(request.getParameter("file"), QByteArray("upload.txt"));
    QCOMPARE(uploadedFile->readAll(), expectedContents);
}
