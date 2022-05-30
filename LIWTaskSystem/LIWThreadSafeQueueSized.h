#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>

namespace LIW {
	namespace Util {
		template<class T, uint64_t Size>
		class LIWThreadSafeQueueSized {
		public:
			typedef uint64_t size_type;
		private:
			typedef std::lock_guard<std::mutex> lock_guard;
			typedef std::unique_lock<std::mutex> uniq_lock;
		public:
			LIWThreadSafeQueueSized() = default;
			LIWThreadSafeQueueSized(const LIWThreadSafeQueueSized&) = delete;
			LIWThreadSafeQueueSized& operator=(const LIWThreadSafeQueueSized&) = delete;

			/// <summary>
			/// Push a value into queue now. 
			/// </summary>
			/// <param name="val"> Value to enqueue. </param>
			/// <returns> Sucessful or not </returns>
			inline bool push_now(const T& val) {
				lock_guard lk(__m_mtx_data);
				if (__m_back.load(std::memory_order_relaxed) - __m_front.load(std::memory_order_relaxed) < Size) {
					size_type back = __m_back.fetch_add(1, std::memory_order_release);
					__m_queue[back % Size] = val;
					__m_cv_nonempty.notify_one();
				}
				else
				{
					return false;
				}
			}
			inline bool push_now(T&& val) {
				lock_guard lk(__m_mtx_data);
				if (__m_back.load(std::memory_order_relaxed) - __m_front.load(std::memory_order_relaxed) < Size) {
					size_type back = __m_back.fetch_add(1, std::memory_order_release);
					__m_queue[back % Size] = val;
					__m_cv_nonempty.notify_one();
				}
				else {
					return false;
				}
			}

			inline void push(const T& val) {
				uniq_lock lk_full(__m_mtx_data);
				while (__m_back.load(std::memory_order_relaxed) - __m_front.load(std::memory_order_relaxed) < Size && __m_running) {
					__m_cv_nonfull.wait(lk_full);
				}
				if (__m_running) {
					size_type back = __m_back.fetch_add(1, std::memory_order_release);
					__m_queue[back % Size] = val;
					__m_cv_nonempty.notify_one();
				}
			}
			inline void push(T&& val) {
				uniq_lock lk_full(__m_mtx_data);
				while (__m_back.load(std::memory_order_relaxed) - __m_front.load(std::memory_order_relaxed) < Size && __m_running) {
					__m_cv_nonfull.wait(lk_full);
				}
				if (__m_running) {
					size_type back = __m_back.fetch_add(1, std::memory_order_release);
					__m_queue[back % Size] = val;
					__m_cv_nonempty.notify_one();
				}
			}

			/// <summary>
			/// Pop from queue immediately. 
			/// </summary>
			/// <param name="valOut"> Value dequeued. </param>
			/// <returns> Is operation successful. </returns>
			bool pop_now(T& valOut) {
				lock_guard lk(__m_mtx_data);
				if (__m_back.load(std::memory_order_relaxed) == __m_front.load(std::memory_order_relaxed)) {
					size_type front = __m_front.fetch_add(1, std::memory_order_release);
					valOut = std::move(__m_queue[front % Size]);
					__m_cv_nonfull.notify_one();
					return true;
				}
				else {
					return false;
				}
			}

			/// <summary>
			/// Pop from queue when not empty. 
			/// </summary>
			/// <param name="valOut"> Value dequeued. </param>
			/// <returns> Is operation successful. </returns>
			int pop(T& valOut) {
				uniq_lock lock_empty(__m_mtx_data);
				while (__m_back.load(std::memory_order_relaxed) == __m_front.load(std::memory_order_relaxed) && __m_running) {
					__m_cv_nonempty.wait(lock_empty);
				}
				if (__m_running) {
					size_type front = __m_front.fetch_add(1, std::memory_order_release);
					valOut = std::move(__m_queue[front % Size]);
					__m_cv_nonfull.notify_one();
					return true;
				}
				else
				{
					return false;
				}
			}

			/// <summary>
			/// Get a copy of the front of the queue. 
			/// </summary>
			/// <param name="valOut"> Copy of the front element. </param>
			/// <returns> Is operation successful. </returns>
			inline bool front(T& valOut) {
				lock_guard lk(__m_mtx_data);
				if (__m_front.load(std::memory_order_acquire) != __m_back.load(std::memory_order_acquire)) {
					valOut = __m_queue[__m_front.load(std::memory_order_relaxed) % Size];
					return true;
				}
				else {
					return false;
				}
			}
			/// <summary>
			/// Get a copy of the end of the queue. 
			/// </summary>
			/// <param name="valOut"> Copy of the last element. </param>
			/// <returns> Is operation successful. </returns>
			inline bool back(T& valOut) {
				lock_guard lk(__m_mtx_data);
				if (__m_front.load(std::memory_order_acquire) != __m_back.load(std::memory_order_acquire)) {
					valOut = __m_queue[__m_back.load(std::memory_order_relaxed) - 1 % Size];
					return true;
				}
				else {
					return false;
				}
			}
			/// <summary>
			/// Get size of queue. 
			/// </summary>
			/// <returns> Size of queue. </returns>
			inline typename size_type size() const { return __m_back.load(std::memory_order_acquire) - __m_front.load(std::memory_order_acquire); }
			/// <summary>
			/// Get if queue is empty. 
			/// </summary>
			/// <returns> Is queue empty. </returns>
			inline bool empty() const { return __m_back.load(std::memory_order_acquire) == __m_front.load(std::memory_order_acquire); }

			/// <summary>
			/// Block the thread until queue is empty. 
			/// </summary>
			inline void block_till_empty() {
				while (!empty()) { std::this_thread::yield(); }
			}
			/// <summary>
			/// Notify all pop() calls to stop blocking and exit. 
			/// </summary>
			inline void notify_stop() {
				__m_running = false;
				__m_cv_nonempty.notify_all();
			}

		protected:
			T __m_queue[Size];
			std::atomic<size_type> __m_front;
			std::atomic<size_type> __m_back;
		private:
			mutable std::mutex __m_mtx_data;
			std::condition_variable __m_cv_nonempty;
			std::condition_variable __m_cv_nonfull;
			bool __m_running = true;
		};
	}
}