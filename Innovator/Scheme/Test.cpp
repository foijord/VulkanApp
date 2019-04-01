#include <QtTest/QtTest>
#include <Innovator/Scheme/Scheme.h>

#include <string>
#include <iostream>

class Test : public QObject
{
  Q_OBJECT

private:

private slots:
  void initTestCase()
  {
    qDebug("Called before everything else.");
  }

  void testTokenizer()
  {
  }

  void cleanupTestCase()
  {
    qDebug("Called after tests.");
  }
};

QTEST_MAIN(Test)
#include "Test.moc"