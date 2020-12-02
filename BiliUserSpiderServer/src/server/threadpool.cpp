#include "threadpool.h"
#include <sys/epoll.h>
#include <sys/fcntl.h>
#include <sys/sysinfo.h>
#include <unistd.h>

#include <errno.h>
#include <string.h>
#include "../log/logger.h"

Task::Task(void (*fn_ptr)(void*), void* arg) {
    m_fn_ptr = fn_ptr;
    m_arg = arg;
}

Task::~Task() {}

void Task::run() {
   (*m_fn_ptr)(m_arg);
}

ThreadPool::ThreadPool() {
    m_scb = NULL;
    m_exit_cb = NULL;
    m_task_size_limit = -1;
    m_pool_size = 0;
    m_pool_state = -1;
}

ThreadPool::~ThreadPool() {
    // Release resources
    if (m_pool_state == STARTED) {
        destroy_threadpool();
    }
}

// We can't pass a member function to pthread_create.
// So created the wrapper function that calls the member function
// we want to run in the thread.
extern "C"
void* ss_start_thread(void* arg) {
    ThreadPool* tp = (ThreadPool *)arg;
    if (tp->m_scb != NULL) {
        tp->m_scb();
    } else {
        LDebug("thread start cb is null");
    }
    tp->execute_thread();
    return NULL;
}

int ThreadPool::start() {
    LWarn("plz use start_threadpool method instead!");
    return start_threadpool();
}

int ThreadPool::start_threadpool() {
    if (m_pool_size == 0) {
        LError("pool size must be set!");
        return -1;
    }
    if (m_pool_state == STARTED) {
        LWarn("ThreadPool has started!");
        return 0;
    }
    m_pool_state = STARTED;
    int ret = -1;
    for (int i = 0; i < m_pool_size; i++) {
        pthread_t tid;
        ret = pthread_create(&tid, NULL, ss_start_thread, (void*) this);
        if (ret != 0) {
            LError("pthread_create() failed: {}", ret);
            return -1;
        }
        m_threads.push_back(tid);
    }
    LDebug("{} threads created by the thread pool", m_pool_size);

    return 0;
}

void ThreadPool::set_thread_start_cb(ThreadStartCallback f) {
    m_scb = f;
}

void ThreadPool::set_thread_exit_cb(ThreadExitCallback f) {
    m_exit_cb = f;
}

void ThreadPool::set_task_size_limit(int size) {
    m_task_size_limit = size;
}

void ThreadPool::set_pool_size(int pool_size) {
    m_pool_size = pool_size;
}

int ThreadPool::destroy_threadpool() {
    // Note: this is not for synchronization, its for thread communication!
    // destroy_threadpool() will only be called from the main thread, yet
    // the modified m_pool_state may not show up to other threads until its 
    // modified in a lock!
    m_task_mutex.lock();
    m_pool_state = STOPPED;
    m_task_mutex.unlock();
    LInfo("Broadcasting STOP signal to all threads...");
    m_task_cond_var.broadcast(); // notify all threads we are shttung down

    int ret = -1;
    for (int i = 0; i < m_pool_size; i++) {
        void* result;
        ret = pthread_join(m_threads[i], &result);
        LDebug("pthread_join() returned {}", ret);
        m_task_cond_var.broadcast(); // try waking up a bunch of threads that are still waiting
    }
    LInfo("{} threads exited from the thread pool, task size:{}", 
            m_pool_size, m_tasks.size());
    return m_tasks.size();
}

void* ThreadPool::execute_thread() {
    Task *task;
    LDebug("Starting thread :{}", pthread_self());
    while(true) {
        // Try to pick a task
        LDebug("Locking: {}", pthread_self());
        m_task_mutex.lock();

        // We need to put pthread_cond_wait in a loop for two reasons:
        // 1. There can be spurious wakeups (due to signal/ENITR)
        // 2. When mutex is released for waiting, another thread can be waken up
        //    from a signal/broadcast and that thread can mess up the condition.
        //    So when the current thread wakes up the condition may no longer be
        //    actually true!
        while ((m_pool_state != STOPPED) && (m_tasks.empty())) {
            // Wait until there is a task in the queue
            // Unlock mutex while wait, then lock it back when signaled
            LDebug("Unlocking and waiting: {}", pthread_self());
            m_task_cond_var.wait(m_task_mutex.get_mutex_ptr());
            LDebug("Signaled and locking: {}", pthread_self());
        }

        // If the thread was woken up to notify process shutdown, return from here
        if (m_pool_state == STOPPED) {
            LInfo("Unlocking and exiting: {}", pthread_self());
            m_task_mutex.unlock();
            if (m_exit_cb != NULL) {
                m_exit_cb();
            }
            pthread_exit(NULL);
        }

        task = m_tasks.front();
        m_tasks.pop_front();
        LDebug("Unlocking: {}", pthread_self());
        m_task_mutex.unlock();

        //cout << "Executing thread " << pthread_self() << endl;
        // execute the task
        task->run(); //
        delete task;
        //cout << "Done executing thread " << pthread_self() << endl;
    }
    return NULL;
}

int ThreadPool::add_task(Task *task) {
    m_task_mutex.lock();

    if (m_task_size_limit > 0 && (int) m_tasks.size() > m_task_size_limit) {
        LWarn("task size reach limit:{}", m_task_size_limit);
        m_task_mutex.unlock();
        return -1;
    }
    m_tasks.push_back(task);

    m_task_cond_var.signal(); // wake up one thread that is waiting for a task to be available

    m_task_mutex.unlock();

    return 0;
}
