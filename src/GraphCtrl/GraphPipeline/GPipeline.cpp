/***************************
@Author: Chunel
@Contact: chunel@foxmail.com
@File: Graphic.cpp
@Time: 2021/6/2 10:15 下午
@Desc: 
***************************/

#include <functional>

#include "GPipeline.h"

CGRAPH_NAMESPACE_BEGIN

GPipeline::GPipeline() {
    thread_pool_ = UThreadPoolSingleton::get();
    element_manager_ = CGRAPH_SAFE_MALLOC_COBJECT(GElementManager)
    param_manager_ = CGRAPH_SAFE_MALLOC_COBJECT(GParamManager)
    is_init_ = false;
}


GPipeline::~GPipeline() {
    CGRAPH_DELETE_PTR(element_manager_)
    CGRAPH_DELETE_PTR(param_manager_)

    // 结束的时候，清空所有创建的节点信息。所有节点信息，仅在这一处释放
    for (GElementPtr element : element_repository_) {
        CGRAPH_DELETE_PTR(element)
    }
}


CStatus GPipeline::init() {
    CGRAPH_FUNCTION_BEGIN
    CGRAPH_ASSERT_NOT_NULL(thread_pool_)
    CGRAPH_ASSERT_NOT_NULL(element_manager_)
    CGRAPH_ASSERT_NOT_NULL(param_manager_)

    status = element_manager_->init();
    CGRAPH_FUNCTION_CHECK_STATUS

    is_init_ = true;
    CGRAPH_FUNCTION_END
}


CStatus GPipeline::run() {
    CGRAPH_FUNCTION_BEGIN

    CGRAPH_ASSERT_INIT(true)
    CGRAPH_ASSERT_NOT_NULL(thread_pool_)
    CGRAPH_ASSERT_NOT_NULL(element_manager_)
    CGRAPH_ASSERT_NOT_NULL(param_manager_)

    int runElementSize = 0;    // 用于记录执行的element的总数，用于后期校验
    std::vector<int> curClusterTtl;    // 用于记录分解后，每个cluster包含的element的个数，用于验证执行的超时情况。
    std::vector<std::future<CStatus>> futures;

    for (GClusterArrRef clusterArr : element_manager_->para_cluster_arrs_) {
        futures.clear();
        curClusterTtl.clear();

        /** 将分解后的pipeline信息，以cluster为维度，放入线程池依次执行 */
        for (GClusterRef cluster : clusterArr) {
            futures.emplace_back(thread_pool_->commit(std::bind(&GCluster::process, std::ref(cluster), false)));
            runElementSize += cluster.getElementNum();
            curClusterTtl.emplace_back(cluster.getElementNum() * element_run_ttl_);
        }

        int index = 0;
        for (auto& fut : futures) {
            if (likely(DEFAULT_ELEMENT_RUN_TTL == element_run_ttl_)) {
                status += fut.get();    // 不设定最大运行周期的情况（默认情况）
            } else {
                const auto& futStatus = fut.wait_for(std::chrono::milliseconds(curClusterTtl[index]));
                switch (futStatus) {
                    case std::future_status::ready: status += fut.get(); break;
                    case std::future_status::timeout: status += CStatus("thread status timeout"); break;
                    case std::future_status::deferred: status += CStatus("thread status deferred"); break;
                    default: status += CStatus("thread status unknown");
                }
                index++;
            }
        }
        CGRAPH_FUNCTION_CHECK_STATUS
    }

    param_manager_->reset();
    status = element_manager_->afterRunCheck(runElementSize);
    CGRAPH_FUNCTION_END
}


CStatus GPipeline::deinit() {
    CGRAPH_FUNCTION_BEGIN

    status = element_manager_->deinit();
    CGRAPH_FUNCTION_CHECK_STATUS

    status = param_manager_->deinit();
    CGRAPH_FUNCTION_END
}


CStatus GPipeline::process(int runTimes) {
    CGRAPH_FUNCTION_BEGIN
    status = init();
    CGRAPH_FUNCTION_CHECK_STATUS

    while (runTimes-- > 0) {
        status = run();
        CGRAPH_FUNCTION_CHECK_STATUS
    }

    status = deinit();
    CGRAPH_FUNCTION_END
}


GPipelinePtr GPipeline::setElementRunTtl(int ttl) {
    if (is_init_) {
        return nullptr;    // 初始化前才可设置
    }

    this->element_run_ttl_ = ttl > 0 ? ttl : DEFAULT_ELEMENT_RUN_TTL;
    return this;
}

CGRAPH_NAMESPACE_END
