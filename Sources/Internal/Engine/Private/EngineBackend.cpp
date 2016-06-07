#if defined(__DAVAENGINE_COREV2__)

#include "Engine/Public/Engine.h"

#include "Engine/Public/AppContext.h"
#include "Engine/Private/EngineBackend.h"
#include "Engine/Private/WindowBackend.h"
#include "Engine/Private/PlatformCore.h"
#include "Engine/Private/Dispatcher/Dispatcher.h"

#include "Render/Renderer.h"
#include "Render/2D/Systems/VirtualCoordinatesSystem.h"
#include "Render/2D/Systems/RenderSystem2D.h"

#include "Logger/Logger.h"
#include "DAVAClassRegistrator.h"
#include "FileSystem/FileSystem.h"
#include "Base/ObjectFactory.h"
#include "Core/ApplicationCore.h"
#include "Core/Core.h"
#include "Core/PerformanceSettings.h"
#include "Platform/SystemTimer.h"
#include "UI/UIScreenManager.h"
#include "UI/UIControlSystem.h"
#include "Input/InputSystem.h"
#include "Debug/DVAssert.h"
#include "Render/2D/TextBlock.h"
#include "Debug/Replay.h"
#include "Sound/SoundSystem.h"
#include "Sound/SoundEvent.h"
#include "Input/InputSystem.h"
#include "Platform/DPIHelper.h"
#include "Base/AllocatorFactory.h"
#include "Render/2D/FTFont.h"
#include "Scene3D/SceneFile/VersionInfo.h"
#include "Render/Image/ImageSystem.h"
#include "Render/2D/Systems/VirtualCoordinatesSystem.h"
#include "Render/2D/Systems/RenderSystem2D.h"
#include "DLC/Downloader/DownloadManager.h"
#include "DLC/Downloader/CurlDownloader.h"
#include "Render/OcclusionQuery.h"
#include "Notification/LocalNotificationController.h"
#include "Platform/DeviceInfo.h"
#include "Render/Renderer.h"
#include "UI/UIControlSystem.h"
#include "Job/JobManager.h"

#include "UI/UIEvent.h"

namespace DAVA
{
namespace Private
{
EngineBackend* EngineBackend::instance = nullptr;

EngineBackend::EngineBackend(int argc, char* argv[])
    : dispatcher(new Dispatcher)
    , platformCore(new PlatformCore)
    , context(new AppContext)
{
    DVASSERT(instance == nullptr);
    instance = this;

    new Logger;

    windows.reserve(10);
    cmdargs = platformCore->GetCommandLine(argc, argv);
}

EngineBackend::~EngineBackend()
{
    delete context;
    delete dispatcher;
    delete platformCore;
    instance = nullptr;
}

void EngineBackend::Init(bool consoleMode_, const Vector<String>& modules)
{
    Logger::Debug("****** EngineBackend::Init enter");

    consoleMode = consoleMode_;

    platformCore->Init();
    if (!consoleMode)
    {
#if defined(__DAVAENGINE_QT__)
        ;
#elif defined(__DAVAENGINE_WIN32__)
        primaryWindow = new WindowBackend(this, true);
        windows.push_back(primaryWindow);
#endif
    }

    // init modules

    new AllocatorFactory();
    new FileSystem();
    FilePath::InitializeBundleName();
    FileSystem::Instance()->SetDefaultDocumentsDirectory();
    FileSystem::Instance()->CreateDirectory(FileSystem::Instance()->GetCurrentDocumentsDirectory(), true);

    if (!consoleMode)
    {
        Logger::Info("SoundSystem init start");
        new SoundSystem();
        Logger::Info("SoundSystem init finish");

        DeviceInfo::InitializeScreenInfo();
        new AnimationManager();
        new FontManager();
        new UIControlSystem();
        new InputSystem();
        new FrameOcclusionQueryManager();
        new VirtualCoordinatesSystem();
        new RenderSystem2D();
        new UIScreenManager();

        new LocalNotificationController();
    }

    new LocalizationSystem();
    new SystemTimer();
    new Random();
    new PerformanceSettings();
    new VersionInfo();

    Thread::InitMainThread();

    new DownloadManager();
    DownloadManager::Instance()->SetDownloader(new CurlDownloader());

    context->jobManager = new JobManager(engine);

    context->inputSystem = InputSystem::Instance();
    context->uiControlSystem = UIControlSystem::Instance();
    context->virtualCoordSystem = VirtualCoordinatesSystem::Instance();

    RegisterDAVAClasses();

    if (!consoleMode_)
    {
        VirtualCoordinatesSystem::Instance()->SetVirtualScreenSize(1024, 768);
        VirtualCoordinatesSystem::Instance()->RegisterAvailableResourceSize(1024, 768, "Gfx");
    }

    Logger::Debug("****** EngineBackend::Init leave");
}

int EngineBackend::Run()
{
    if (consoleMode)
    {
        RunConsole();
    }
    else
    {
#if defined(__DAVAENGINE_QT__)
        ;
#elif defined(__DAVAENGINE_WIN32__)
        platformCore->CreateNativeWindow(primaryWindow);
#endif
        platformCore->Run();
    }
    return exitCode;
}

void EngineBackend::Quit(int exitCode_)
{
    exitCode = exitCode_;
    if (consoleMode)
        quitConsole = true;
    else
        platformCore->Quit();
}

void EngineBackend::RunAsyncOnMainThread(const Function<void()>& task)
{
    DispatcherEvent e;
    e.type = DispatcherEvent::FUNCTOR;
    e.window = nullptr;
    e.functor = task;
    dispatcher->PostEvent(e);
}

void EngineBackend::RunConsole()
{
    OnGameLoopStarted();
    while (!quitConsole)
    {
        OnFrameConsole();
        Thread::Sleep(1);
    }
    OnGameLoopStopped();
}

void EngineBackend::OnGameLoopStarted()
{
    engine->gameLoopStarted.Emit();
}

void EngineBackend::OnGameLoopStopped()
{
    engine->gameLoopStopped.Emit();
    if (!consoleMode)
    {
        Renderer::Uninitialize();
    }
}

void EngineBackend::DoEvents()
{
    dispatcher->ProcessEvents(MakeFunction(this, &EngineBackend::EventHandler));
    for (WindowBackend* w : windows)
    {
        w->FinishEventHandlingOnCurrentFrame();
    }
}

void EngineBackend::OnFrameConsole()
{
    SystemTimer::Instance()->Start();
    float32 frameDelta = SystemTimer::Instance()->FrameDelta();
    SystemTimer::Instance()->UpdateGlobalTime(frameDelta);

    DoEvents();

    DownloadManager::Instance()->Update();

    // JobManager::Update() is invoked through signal update
    engine->update.Emit(frameDelta);
}

int32 EngineBackend::OnFrame()
{
    SystemTimer::Instance()->Start();
    float32 frameDelta = SystemTimer::Instance()->FrameDelta();
    SystemTimer::Instance()->UpdateGlobalTime(frameDelta);

#if defined(__DAVAENGINE_QT__)
    rhi::InvalidateCache();
#endif

    DoEvents();
    OnBeginFrame();
    OnUpdate(frameDelta);
    OnDraw();
    OnEndFrame();

    return Renderer::GetDesiredFPS();
}

void EngineBackend::OnBeginFrame()
{
    Renderer::BeginFrame();

    InputSystem::Instance()->OnBeforeUpdate();
    engine->beginFrame.Emit();
}

void EngineBackend::OnUpdate(float32 frameDelta)
{
    LocalNotificationController::Instance()->Update();
    SoundSystem::Instance()->Update(frameDelta);
    AnimationManager::Instance()->Update(frameDelta);
    DownloadManager::Instance()->Update();

    for (WindowBackend* w : windows)
    {
        w->Update(frameDelta);
    }
    // JobManager::Update() is invoked through signal update
    engine->update.Emit(frameDelta);
}

void EngineBackend::OnDraw()
{
    RenderSystem2D::Instance()->BeginFrame();

    FrameOcclusionQueryManager::Instance()->ResetFrameStats();
    for (WindowBackend* w : windows)
    {
        w->Draw();
    }
    FrameOcclusionQueryManager::Instance()->ProccesRenderedFrame();

    engine->draw.Emit();
    RenderSystem2D::Instance()->EndFrame();
}

void EngineBackend::OnEndFrame()
{
    InputSystem::Instance()->OnAfterUpdate();
    engine->endFrame.Emit();
    Renderer::EndFrame();
}

void EngineBackend::EventHandler(const DispatcherEvent& e)
{
    switch (e.type)
    {
    case DispatcherEvent::FUNCTOR:
        e.functor();
        break;
    case DispatcherEvent::WINDOW_CREATED:
        HandleWindowCreated(e);
        break;
    case DispatcherEvent::WINDOW_DESTROYED:
        HandleWindowDestroyed(e);
        break;
    default:
        if (e.window != nullptr)
        {
            e.window->EventHandler(e);
        }
        break;
    }
}

void EngineBackend::HandleWindowCreated(const DispatcherEvent& e)
{
    e.window->EventHandler(e);
    engine->windowCreated.Emit(e.window->GetWindow());
}

void EngineBackend::HandleWindowDestroyed(const DispatcherEvent& e)
{
    engine->windowDestroyed.Emit(e.window->GetWindow());
    e.window->EventHandler(e);
    if (e.window->IsPrimary())
    {
        engine->Quit();
    }
}

void EngineBackend::InitRenderer(WindowBackend* w)
{
    rhi::InitParam rendererParams;
    rhi::Api renderer = rhi::RHI_DX9; //static_cast<rhi::Api>(options->GetInt32("renderer"));

    rendererParams.threadedRenderEnabled = true;
    rendererParams.threadedRenderFrameCount = 2;

    rendererParams.maxIndexBufferCount = options->GetInt32("max_index_buffer_count");
    rendererParams.maxVertexBufferCount = options->GetInt32("max_vertex_buffer_count");
    rendererParams.maxConstBufferCount = options->GetInt32("max_const_buffer_count");
    rendererParams.maxTextureCount = options->GetInt32("max_texture_count");

    rendererParams.maxTextureSetCount = options->GetInt32("max_texture_set_count");
    rendererParams.maxSamplerStateCount = options->GetInt32("max_sampler_state_count");
    rendererParams.maxPipelineStateCount = options->GetInt32("max_pipeline_state_count");
    rendererParams.maxDepthStencilStateCount = options->GetInt32("max_depthstencil_state_count");
    rendererParams.maxRenderPassCount = options->GetInt32("max_render_pass_count");
    rendererParams.maxCommandBuffer = options->GetInt32("max_command_buffer_count");
    rendererParams.maxPacketListCount = options->GetInt32("max_packet_list_count");

    rendererParams.shaderConstRingBufferSize = options->GetInt32("shader_const_buffer_size");

    rendererParams.window = w->GetNativeHandle();
    rendererParams.width = static_cast<int>(w->GetWidth());
    rendererParams.height = static_cast<int>(w->GetHeight());
    rendererParams.scaleX = w->GetScaleX();
    rendererParams.scaleY = w->GetScaleY();

    rhi::ShaderSourceCache::Load("~doc:/ShaderSource.bin");
    Renderer::Initialize(renderer, rendererParams);
    RenderSystem2D::Instance()->Init();
}

void EngineBackend::ResetRenderer(WindowBackend* w, bool resetToNull)
{
    rhi::ResetParam rendererParams;
    if (resetToNull)
    {
        rendererParams.window = nullptr;
        rendererParams.width = 0;
        rendererParams.height = 0;
        rendererParams.scaleX = 0.f;
        rendererParams.scaleY = 0.f;
    }
    else
    {
        rendererParams.window = w->GetNativeHandle();
        rendererParams.width = static_cast<int>(w->GetWidth());
        rendererParams.height = static_cast<int>(w->GetHeight());
        rendererParams.scaleX = w->GetScaleX();
        rendererParams.scaleY = w->GetScaleY();
    }
    Renderer::Reset(rendererParams);
}

void EngineBackend::DeinitRender(WindowBackend* w)
{
}

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_COREV2__
