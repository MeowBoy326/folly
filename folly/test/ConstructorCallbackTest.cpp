/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdexcept>
#include <folly/ConstructorCallback.h>
#include <folly/portability/GTest.h>

using namespace ::testing;

namespace {
class Foo {
 public:
  int i_;
  explicit Foo(int i) : i_{i} {}

 private:
  folly::ConstructorCallback<Foo> constructorCallback_{this};
};

constexpr int kBarSize = 7;
class Bar {
 public:
  int i_;
  explicit Bar(int i) : i_{i} {}

 private:
  // same as Foo but with non-default Callback size
  folly::ConstructorCallback<Bar, kBarSize> constructorCallback_{this};
};
} // namespace

TEST(ConstructorCallbackTest, basic) {
  int count = 0;
  int lastI = -1;
  auto callbackF = [&](Foo* f) {
    count++;
    lastI = f->i_;
  };

  Foo f1{88}; // no call back called
  EXPECT_EQ(count, 0);
  EXPECT_EQ(lastI, -1);

  // add callback, verify call
  folly::ConstructorCallback<Foo>::addNewConstructorCallback(callbackF);
  Foo f2{99};

  EXPECT_EQ(count, 1);
  EXPECT_EQ(lastI, 99);
}

TEST(ConstructorCallbackTest, overflow) {
  int count = 0;
  int lastI = -1;
  auto callbackF = [&](Foo* f) {
    count++;
    lastI = f->i_;
  };

  // add one too many to the call backs
  for (std::size_t i = 0;
       i < folly::ConstructorCallback<Foo>::kMaxCallbacks + 1;
       i++) {
    // add callback multiple times
    if (i < folly::ConstructorCallback<Foo>::kMaxCallbacks) {
      // every other time should work without throwing the exception
      folly::ConstructorCallback<Foo>::addNewConstructorCallback(callbackF);
    } else {
      // last add should fail
      EXPECT_THROW(
          folly::ConstructorCallback<Foo>::addNewConstructorCallback(callbackF),
          std::length_error);
    }
  }
  Foo f{99};
  EXPECT_EQ(count, folly::ConstructorCallback<Foo>::kMaxCallbacks);
  EXPECT_EQ(lastI, 99);
}

TEST(ConstructorCallbackTest, overflow7) {
  int count = 0;
  int lastI = -1;
  auto callbackF = [&](Bar* b) {
    count++;
    lastI = b->i_;
  };

  // same as test above, but make sure we can change the size
  // of the callback array from the default

  // add one too many to the call backs
  for (std::size_t i = 0;
       i < folly::ConstructorCallback<Bar, kBarSize>::kMaxCallbacks + 1;
       i++) {
    // add callback multiple times
    if (i == (folly::ConstructorCallback<Bar, kBarSize>::kMaxCallbacks)) {
      // last add should fail
      EXPECT_THROW(
          (folly::ConstructorCallback<Bar, kBarSize>::addNewConstructorCallback(
              callbackF)),
          std::length_error);
    } else {
      // every other time should work;
      folly::ConstructorCallback<Bar, kBarSize>::addNewConstructorCallback(
          callbackF);
    }
  }
  Bar b{99};
  EXPECT_EQ(count, (folly::ConstructorCallback<Bar, kBarSize>::kMaxCallbacks));
  EXPECT_EQ(lastI, 99);
}

TEST(ConstructorCallbackTest, size) {
  // Verify that adding a ConstructorCallback uses at most 1 byte more memory
  // This will help ensure that this code remains 'lightweight'
  auto ccb = folly::ConstructorCallback<void>(nullptr);
  EXPECT_LE(sizeof(ccb), 1);
}