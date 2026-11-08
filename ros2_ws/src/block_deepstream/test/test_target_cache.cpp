#include "target_cache.hpp"
#include <future>
#include <iostream>
#include <stdexcept>
#include <thread>
#define CHECK(x) do { if (!(x)) throw std::runtime_error(#x); } while (0)
int main() {
  TargetCache cache;
  TargetCache::Target target;
  target.header.stamp.sec = 10; target.header.frame_id = "camera"; target.track_id = 42;
  target.valid = true; target.image_width = 1280; target.image_height = 720;
  cache.put(target);
  auto wrong = target.header; wrong.frame_id = "other";
  CHECK(!cache.wait(wrong, std::chrono::milliseconds(0)));
  wrong = target.header; wrong.stamp.nanosec = 1;
  CHECK(!cache.wait(wrong, std::chrono::milliseconds(0)));
  auto matched = cache.wait(target.header, std::chrono::milliseconds(0));
  CHECK(matched && matched->track_id == 42);
  CHECK(!cache.wait(target.header, std::chrono::milliseconds(0)));
  auto start = std::chrono::steady_clock::now();
  CHECK(!cache.wait(target.header, std::chrono::milliseconds(20)));
  CHECK(std::chrono::steady_clock::now() - start < std::chrono::milliseconds(100));
  std::thread arrival([&] {
    std::this_thread::sleep_for(std::chrono::milliseconds(5)); cache.put(target);
  });
  auto late = cache.wait(target.header, std::chrono::milliseconds(50));
  arrival.join();
  CHECK(late && late->track_id == 42);
  for (int i = 0; i < 70; ++i) { target.header.stamp.sec = i; cache.put(target); }
  auto old = target.header; old.stamp.sec = 0;
  CHECK(!cache.wait(old, std::chrono::milliseconds(0)));
  auto pending = std::async(std::launch::async, [&] { return cache.wait(old, std::chrono::seconds(2)); });
  cache.stop();
  CHECK(pending.wait_for(std::chrono::milliseconds(100)) == std::future_status::ready);
  CHECK(!pending.get());
  std::cout << "Exact frame matching, bounded cache, timeout and shutdown passed\n";
}
