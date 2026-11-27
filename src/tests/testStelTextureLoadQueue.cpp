/*
 * Stellarium
 * Copyright (C) 2026 KainanP
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

#include "tests/testStelTextureLoadQueue.hpp"
#include "StelTextureLoadQueue.hpp"

QTEST_GUILESS_MAIN(TestStelTextureLoadQueue)

namespace
{
// Stands for StelTexture: only the bind request counter matters to the queue.
struct FakeTexture
{
	explicit FakeTexture(const QString& name) : name(name) {}
	unsigned getBindRequestCount() const { return bindRequests; }
	// What StelTexture::bind() does while the texture is not loaded yet
	void bindFailed(unsigned times = 1) { bindRequests += times; }

	QString name;
	unsigned bindRequests = 0;
};
using FakeTextureSP = QSharedPointer<FakeTexture>;

FakeTextureSP make(const QString& name) { return FakeTextureSP::create(name); }

QStringList drain(StelTextureLoadQueue<FakeTexture>& queue)
{
	QStringList order;
	while (const auto tex = queue.takeNext())
		order << tex->name;
	return order;
}
}

void TestStelTextureLoadQueue::equalRequestsKeepInsertionOrder()
{
	StelTextureLoadQueue<FakeTexture> queue;
	const auto a = make("a"), b = make("b"), c = make("c");
	queue.push(a);
	queue.push(b);
	queue.push(c);
	QCOMPARE(drain(queue), QStringList({"a", "b", "c"}));
	QVERIFY(queue.isEmpty());
}

void TestStelTextureLoadQueue::mostRequestedComesFirst()
{
	// Many deep-sky textures were requested once while zooming in; then the planet
	// in the middle of the screen is requested in every frame.
	StelTextureLoadQueue<FakeTexture> queue;
	QList<FakeTextureSP> nebulae;
	for (int i = 0; i < 5; ++i)
	{
		nebulae << make(QString("nebula%1").arg(i));
		nebulae.last()->bindFailed();
		queue.push(nebulae.last());
	}
	const auto planet = make("planet");
	planet->bindFailed(10);
	queue.push(planet);

	const QStringList order = drain(queue);
	QCOMPARE(order.first(), QString("planet"));
	QCOMPARE(order.mid(1), QStringList({"nebula0", "nebula1", "nebula2", "nebula3", "nebula4"}));
}

void TestStelTextureLoadQueue::requestsAfterPushAreAccounted()
{
	// Priority is evaluated when taking, not when pushing
	StelTextureLoadQueue<FakeTexture> queue;
	const auto early = make("early"), late = make("late");
	queue.push(early);
	queue.push(late);
	late->bindFailed(3);
	early->bindFailed(1);
	QCOMPARE(queue.highestBindRequestCount(), 3u);
	QCOMPARE(queue.takeNext()->name, QString("late"));
	early->bindFailed(5);
	QCOMPARE(queue.takeNext()->name, QString("early"));
	QVERIFY(queue.takeNext().isNull());
}

void TestStelTextureLoadQueue::destroyedTexturesAreDropped()
{
	StelTextureLoadQueue<FakeTexture> queue;
	auto gone = make("gone");
	gone->bindFailed(100);
	const auto kept = make("kept");
	queue.push(gone);
	queue.push(kept);
	gone.reset();
	QCOMPARE(queue.highestBindRequestCount(), 0u);
	QCOMPARE(drain(queue), QStringList({"kept"}));
	QVERIFY(queue.isEmpty());
}

void TestStelTextureLoadQueue::removeAndHighestCount()
{
	StelTextureLoadQueue<FakeTexture> queue;
	QCOMPARE(queue.highestBindRequestCount(), 0u);
	const auto a = make("a"), b = make("b");
	a->bindFailed(7);
	b->bindFailed(2);
	queue.push(a);
	queue.push(b);
	QCOMPARE(queue.highestBindRequestCount(), 7u);
	queue.remove(a.data());
	QCOMPARE(queue.size(), 1);
	QCOMPARE(queue.highestBindRequestCount(), 2u);
	QCOMPARE(drain(queue), QStringList({"b"}));
}
