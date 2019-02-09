#include <QtTest/QtTest>
#include <Innovator/Core/Scheme/Scheme.h>

#include <string>
#include <iostream>

class Test : public QObject
{
  Q_OBJECT

private:
  Scheme scheme;

private slots:
  void initTestCase()
  {
    qDebug("Called before everything else.");
  }

  void testTokenizer()
  {
    std::string input("() + - * / symbol symbol_underscore 0 1 2 3 4 5 6 7 8 9");
    std::vector<std::string> expected_tokens({ 
      "(", ")", "+", "-", "*", "/", "symbol", "symbol_underscore", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" 
    });

    std::sregex_token_iterator tokens(input.begin(), input.end(), this->scheme.tokenizer);

    for (auto expected_token : expected_tokens) {
      QCOMPARE(tokens->str(), expected_token);
      tokens++;
    }
  }

  void cleanupTestCase()
  {
    qDebug("Called after tests.");
  }
};

QTEST_MAIN(Test)
#include "Test.moc"