/***************************
@Author: Chunel
@Contact: chunel@foxmail.com
@File: Graphic.h
@Time: 2021/6/2 10:15 下午
@Desc: 
***************************/

#ifndef CGRAPH_GPIPELINE_H
#define CGRAPH_GPIPELINE_H

#include <vector>
#include <memory>
#include <list>

#include "../GraphObject.h"
#include "GPipelineDefine.h"

CGRAPH_NAMESPACE_BEGIN

class GPipeline : public GraphObject {
public:
    /**
     * 初始化pipeline信息
     * @return
     */
    CStatus init() override;

    /**
     * 执行pipeline信息
     * @return
     */
    CStatus run() override;

    /**
     * 逆初始化pipeline信息
     * @return
     */
    CStatus deinit() override;

    /**
     * 一次性执行完成初始化，执行runTimes次，和逆初始化的过程
     * @param runTimes
     * @return
     */
    CStatus process(int runTimes = DEFAULT_LOOP_TIMES);

    /**
     * 根据传入的info信息，创建node节点
     * @tparam T
     * @param info
     * @return
     */
    template<typename T, std::enable_if_t<std::is_base_of_v<GNode, T>, int> = 0>
    GNodePtr createGNode(const GNodeInfo &info);

    /**
     * 根据传入的信息，创建Group信息
     * @tparam T
     * @param elements
     * @param dependElements
     * @param name
     * @param loop
     * @return
     */
    template<typename T, std::enable_if_t<std::is_base_of_v<GGroup, T>, int> = 0>
    GGroupPtr createGGroup(const GElementPtrArr &elements,
                           const GElementPtrSet &dependElements = std::initializer_list<GElementPtr>(),
                           const std::string &name = "",
                           int loop = DEFAULT_LOOP_TIMES);

    /**
     * 在图中注册一个Element信息
     * 如果注册的是GNode信息，则内部自动生成
     * 如果注册的是GGroup信息，则需外部提前生成，然后注册进来
     * @tparam T
     * @param elementRef
     * @param dependElements
     * @param name
     * @param loop
     * @return
     */
    template<typename T, std::enable_if_t<std::is_base_of_v<GElement, T>, int> = 0>
    CStatus registerGElement(GElementPtr *elementRef,
                             const GElementPtrSet &dependElements = std::initializer_list<GElementPtr>(),
                             const std::string &name = "",
                             int loop = DEFAULT_LOOP_TIMES);

    /**
     * 添加参数，pipeline中所有节点共享此参数
     * @tparam T
     * @param key
     * @return
     */
    template<typename T, std::enable_if_t<std::is_base_of_v<GParam, T>, int> = 0>
    GPipeline* addGParam(const std::string& key);

    /**
     * 批量添加切面
     * @tparam T
     * @param elements
     * @return
     */
    template<typename TAspect, typename TParam = GAspectDefaultParam,
            std::enable_if_t<std::is_base_of_v<GAspect, TAspect>, int> = 0,
            std::enable_if_t<std::is_base_of_v<GAspectParam, TParam>, int> = 0>
    GPipeline* addGAspectBatch(const GElementPtrSet& elements = std::initializer_list<GElementPtr>(),
                               TParam* param = nullptr);

    /**
     * 设置执行的最大时间周期，单位为毫秒
     * @param ttl
     * @return
     */
    GPipeline* setElementRunTtl(int ttl);

protected:
    explicit GPipeline();
    ~GPipeline() override;

private:
    bool is_init_ = false;                               // 初始化标志位
    int element_run_ttl_ = DEFAULT_ELEMENT_RUN_TTL;      // 单个节点最大运行周期
    GElementManagerPtr element_manager_;                 // 节点管理类（管理所有注册过的element信息）
    GElementPtrSet element_repository_;                  // 标记创建的所有节点，最终释放使用
    GParamManagerPtr param_manager_;                     // 参数管理类
    UThreadPoolPtr thread_pool_;                         // 线程池类

    friend class GPipelineFactory;
    friend class UAllocator;
};

using GPipelinePtr = GPipeline *;
using GPipelinePtrList = std::list<GPipelinePtr>;

CGRAPH_NAMESPACE_END

#include "GPipeline.inl"

#endif //CGRAPH_GPIPELINE_H
