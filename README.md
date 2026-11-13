# PodoRenderer
Win32와 D3D12를 이용하여 구현 중인 렌더링 엔진입니다.

실시간 렌더링을 윈도우 데스크톱 환경에 담아내는 것을 목표로 합니다.

현재 프레임 자원을 도입하고, HDR 옵션 기능을 구현하고 있습니다.

<br>




<!---------------------------------------------------------------------------------------------------------------------------------------------->
## 목차
- [1. 스크린샷](#1-스크린샷)
- [2. 실행 방법](#2-실행-방법)
- [3. 주요 기능](#3-주요-기능)
- [4. 성능 측정](#4-성능-측정)
- [5. 구현 제외](#5-구현-제외)
- [6. 외부 항목](#6-외부-항목)
- [7. 구현 참고](#7-구현-참고)

<br>




<!---------------------------------------------------------------------------------------------------------------------------------------------->
## 1. 스크린샷
[준비]
![로딩 스크린샷](Docs/screenshot_1.png)

<br>




[실행]
![실행 스크린샷](Docs/screenshot_2.png)

<br>




<!---------------------------------------------------------------------------------------------------------------------------------------------->
## 2. 실행 방법
### 2.1. 최소 사양
|구분           |최소 사양                            |
|---------------|-------------------------------------|
|운영체제       |Windows 10 22H2 64-bit 이상          |
|GPU 피처 레벨  |D3D_FEATURE_LEVEL_12_0 이상          |
|GPU 셰이더 모델|Shader Model 6.6 이상                |
|GPU 바인딩 티어|D3D12_RESOURCE_BINDING_TIER_3 이상   |

- 운영체제는 지원 범위를 단순화하기 위해 Windows 10의 마지막 기능 업데이트인 22H2를 최소 사양으로 정하였습니다.

- 추후 바인드리스 서술자 힙 접근을 도입할 수 있도록 Shader Model 6.6을 최소 사양으로 정하였습니다.

- 여러 광원 각각을 상수 버퍼 뷰로 묶기 위해 D3D12_RESOURCE_BINDING_TIER_3을 최소 사양으로 정하였습니다.

<br>




### 2.2. 빌드 방법
1. 리포지토리를 다운로드합니다.

2. 다운로드한 리포지토리에 포함된 `PodoRenderer.slnx` 솔루션 파일을 더블 클릭합니다.

3. Visual Studio 상단의 구성을 `Release`로, 플랫폼을 `x64`로 설정합니다.  

4. Visual Studio 상단에서 `빌드(B)`의 `솔루션 빌드(Ctrl+Shift+B)`를 누릅니다.

5. `OutDir/PodoRendererRelease64.exe` 이름의 파일이 생성되었는지 확인합니다.

<br>




### 2.3. 실행 방법
1. 위 빌드 과정을 통해 생성한 `OutDir/PodoRendererRelease64.exe`를 더블 클릭하여 실행합니다.

2. 시작 전 옵션을 설정할 수 있는 준비 화면이 나타납니다. `Start` 버튼을 눌러 시작합니다.

3. 시간에 따라 진동하는 두 개의 정육면체가 화면에 나타납니다.
  
4. 왼쪽 상단의 `End` 버튼을 누르면 다시 준비 화면으로 돌아갑니다.

5. 준비 화면의 `Exit` 버튼을 누르거나 `ALT+F4` 단축키를 눌러 종료합니다.

<br>




<!---------------------------------------------------------------------------------------------------------------------------------------------->
## 3. 주요 기능
### 3.1. 조형 기능
직육면체 메쉬를 이용하여 간단한 공간을 구성하였습니다. 그 중 두 개의 오브젝트는 시간에 따라 움직이도록 하였습니다.

<br>




### 3.2. 조명 기능
여러 개의 방향성 조명과, 각 조명에 대한 기초적인 쉐도우 매핑을 지원합니다. 그리고 디퓨즈 텍스처 기능이 구현되어 있습니다.

<br>




### 3.3. 조작 기능
|입력               |기능                             |
|-------------------|---------------------------------|
|키보드 `ESC`       |실행 상태에서 준비 상태로 복귀   |
|키보드 `ALT+ENTER` |화면 모드 전환                   |

- 사용자의 카메라 조작 기능은 아직 구현되지 않았습니다.

- 대신 카메라를 시간에 따라 공전시키며 워크로드를 측정할 수 있도록 했습니다.

<br>




### 3.4. 옵션 기능
|옵션                   |설정값                                       |
|-----------------------|---------------------------------------------|
|화면 모드              |`창 모드` or `테두리 없는 창 모드`           |
|VSync                  |`Off` or `On`                                |
|Tearing                |피처 지원 여부와 VSync 여부에 따라 자동 적용 |
|HDR                    |`Off` or `On`                                |
|GUI 크기               |`50%` or `75%` or `100%` or `125%` or `150%` |

- 옵션은 `OutDir/SavedSettings.txt` 파일에 저장되며, 다음 실행 시 복원됩니다.

<br>




<!---------------------------------------------------------------------------------------------------------------------------------------------->
## 4. 성능 측정
### 4.1. 측정 환경
|환경 요소          |세부 정보                                                        |
|-------------------|-----------------------------------------------------------------|    
|CPU                |i5-13600KF, 3500 MHz, 14 코어, 20 논리 프로세서                  |
|GPU                |INNO3D GeForce RTX 4070 Ti D6X 12GB X3                           |
|Display            |Samsung Odyssey G6 LS32BG650 QHD 240 Hz                          |
|RAM                |32 GB, DDR4, 3600 Hz                                             |
|OS                 |Windows 11, 버전 25H2, 빌드 26200.8973                           |
|플랫폼 도구 집합   |v145 for Microsoft C++ Build Tools                               |
|컴파일러 버전      |x86용 Microsoft (R) C/C++ 최적화 컴파일러 버전 19.51.36252       |
|구성               |`Profile` (= 기본 `Release` 구성 + `USE_PIX` 전처리기 상수 정의) |

<br>




### 4.2. 측정 방법
1. 빌드 구성을 `Profile`로 설정한 뒤 빌드를 수행하여 `OutDir/PodoRendererProfile64.exe` 실행 파일을 생성합니다.
   
2. 생성된 `OutDir/PodoRendererProfile64.exe` 파일을 우클릭한 뒤 관리자 권한으로 실행합니다.  
  (만일 관리자 권한으로 실행하지 않을시 PIX 측정을 시작할 수 없어, 이후 `Start` 버튼을 누르는 과정에서 오류가 발생합니다.)

3. 준비 화면에서 측정 옵션을 설정합니다. 화면 모드는 전체 화면으로 설정하고, 먼저 VSync는 비활성화합니다.
  
4. `Start` 버튼을 누르면 PIX의 Sequential Timing Capture가 자동으로 시작됩니다.

5. 측정하고 싶은 구간이 끝나면 `End` 버튼을 눌러 캡처를 종료합니다.

6. 생성된 측정 결과 파일인 `OutDir/PodoRendererProfile.wpix`을 더블 클릭하여 PIX에서 엽니다.

7. `Metrics` 탭을 통해 각 이벤트의 소요 시간을 분석합니다.  
  (만일 분석 이벤트가 표시되지 않을 경우, 우측의 `PIX CPU Events`와 `PIX GPU Events`의 항목을 활성화합니다.)

8. 다시 3번 단계로 돌아가 VSync를 활성화한 뒤, 동일한 절차로 측정 및 분석을 수행합니다.

<br>




### 4.3. 측정 결과
[VSync Off]
|구분 |측정 구간                                                                                  |평균 소요 시간 |
|-----|-------------------------------------------------------------------------------------------|--------------:|
|CPU  |1. 프레임 시간                                                                             |354,072 ns     |
|     |├─ 2. GPU 명령 완료 대기                                                                   |84,611 ns      |
|     |├─ 3. 논-렌더 로직                                                                         |4,838 ns       |
|     |└─ 4. 렌더 로직                                                                            |264,398 ns     |
|     |&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;├─ 5. 커맨드 리스트 초기화                             |9,215 ns       |
|     |&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;├─ 6. 그리기 준비                                      |3,382 ns       |
|     |&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;├─ 7. 씬 그리기                                        |16,559 ns      |
|     |&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;│&nbsp;&nbsp;&nbsp;&nbsp;├─ 8. 쉐도우 패스             |11,112 ns      |
|     |&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;│&nbsp;&nbsp;&nbsp;&nbsp;└─ 9. 메인 패스               |5,197 ns       |
|     |&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;├─ 10. GUI 그리기                                      |10,572 ns      |
|     |&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;├─ 11. 제시 준비                                       |1,017 ns       |
|     |&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;├─ 12. 커맨드 리스트 제출                              |59,979 ns      |
|     |&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;└─ 13. 제시                                            |163,020 ns     |
|GPU  |1. 프레임 시간                                                                             |19,531 ns      |
|     |├─ 2. 그리기 준비                                                                          |99 ns          |
|     |├─ 3. 씬 그리기                                                                            |19,393 ns      |
|     |│&nbsp;&nbsp;&nbsp;&nbsp;├─ 4. 쉐도우 패스                                                 |2,886 ns       |
|     |│&nbsp;&nbsp;&nbsp;&nbsp;└─ 5. 메인 패스                                                   |16,504         |
|     |├─ 6. GUI 그리기                                                                           |24 ns          |
|     |└─ 7. 제시 준비                                                                            |3 ns           |

- 총 캡처 시간: 16,155.11 ms

- GPU 명령이 모두 소진될 때까지 CPU를 대기시키는 현 로직으로 인해, '2. GPU 명령 완료 대기' 과정에서 CPU 프레임 시간의 약 20%가 대기 상태로 낭비되고 있습니다.

- '11. 제시' 과정은 현재 CPU 프레임 시간의 약 절반을 차지하고 있지만, 렌더링 워크로드가 무거워지면 고정적인 제시 과정의 비중은 감소할 것으로 예상합니다.

- CPU에서는 쉐도우 패스가, GPU에서는 메인 패스가 상대적으로 더 많은 시간을 소요하고 있습니다. CPU에서 쉐도우 패스는 각 쉐도우 맵에 모든 오브젝트들을 그리는 명령을 작성해야 하며, 반대로 GPU에서 메인 패스는 각 쉐도우 맵을 샘플링해야 하기 때문으로 추정합니다.

<br>




[VSync On]  
|구분 |측정 구간                                                                                  |평균 소요 시간 |
|-----|-------------------------------------------------------------------------------------------|--------------:|
|CPU  |1. 프레임 시간                                                                             |4,147,657 ns   |
|     |├─ 2. GPU 명령 완료 대기                                                                   |3,465,864 ns   |
|     |├─ 3. 논-렌더 로직                                                                         |31,962 ns      |
|     |└─ 4. 렌더 로직                                                                            |647,765 ns     |
|     |&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;├─ 5. 커맨드 리스트 초기화                             |32,353 ns      |
|     |&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;├─ 6. 그리기 준비                                      |9,856 ns       |
|     |&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;├─ 7. 씬 그리기                                        |48,345 ns      |
|     |&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;│&nbsp;&nbsp;&nbsp;&nbsp;├─ 8. 쉐도우 패스             |32,976 ns      |
|     |&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;│&nbsp;&nbsp;&nbsp;&nbsp;└─ 9. 메인 패스               |14,738 ns      |
|     |&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;├─ 10. GUI 그리기                                      |31,252 ns      |
|     |&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;├─ 11. 제시 준비                                       |3,006 ns       |
|     |&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;├─ 12. 커맨드 리스트 제출                              |153,398 ns     |
|     |&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;└─ 13. 제시                                            |367,504 ns     |
|GPU  |1. 프레임 시간                                                                             |1,140,896 ns   |
|     |├─ 2. 그리기 준비                                                                          |4,411 ns       |
|     |├─ 3. 씬 그리기                                                                            |1,135,290 ns   |
|     |│&nbsp;&nbsp;&nbsp;&nbsp;├─ 4. 쉐도우 패스                                                 |103,322 ns     |
|     |│&nbsp;&nbsp;&nbsp;&nbsp;└─ 5. 메인 패스                                                   |1,031,962 ns   |
|     |├─ 6. GUI 그리기                                                                           |1,098 ns       |
|     |└─ 7. 제시 준비                                                                            |17 ns          |

- 총 캡처 시간: 15,508.27 ms

- VSync를 켰으므로 명령 큐에 제출된 Present 명령은 VSyncBlank 타이밍에 처리되고, 이 시점까지 \`FlushCommandQueue()\`가 CPU의 다음 프레임 렌더 시작을 블록하므로, CPU의 '2. GPU 명령 완료 대기' 시간이 늘어납니다.

- 측정 환경의 모니터 주사율은 240Hz이므로 주기는 $\frac{1}{240\ \mathrm{Hz}} \approx 4.17\ \mathrm{ms}$이고 이는 CPU의 프레임 시간 측정값과 거의 맞아떨어집니다.

- VSync에 직접적인 영향을 받지 않는 구간의 시간 소요가 늘어난 원인으로, VSync에 의한 유휴 시간이 증가하며 CPU와 GPU의 스케줄링이나 클럭이 영향을 받았을 가능성을 고려하고 있습니다.

<br>




<!---------------------------------------------------------------------------------------------------------------------------------------------->
## 5. 구현 제외
- **디바이스 소실 감지**  
  디바이스 소실을 감지하는 로직은 구현을 단순화하기 위해 작성하지 않았습니다.

- **윈도우 DPI 설정값 자동 반영**  
  본 엔진은 DPI 설정값에 의한 윈도우의 자동 스케일링으로 화면이 훼손되지 않도록 DPI AWARENESS를 매니페스트에 명시하였습니다. 윈도우의 DPI 설정값은 GUI 크기에 반영하지 않고, 대신 옵션에서 GUI의 배율을 50%, 75%, 100%, 125%, 150% 중에서 선택할 수 있도록 간단히 구현하였습니다.

<br>




<!---------------------------------------------------------------------------------------------------------------------------------------------->
## 6. 외부 항목
개발 및 빌드를 위해 다음 외부 SDK, 라이브러리, 툴킷을 사용합니다.

<br>




### 6.1. DirectX 12 Agility SDK
- 목적
  - 윈도우 업데이트와 상관없이 최신 D3D12 헤더, 런타임 사용

- 사용 방식
  - NuGet 패키지 관리 (Microsoft.Direct3D.D3D12)
  
- 복원 위치
  - `PodoRenderer\packages\Microsoft.Direct3D.D3D12.<버전명>`

- 라이선스 종류
  - MICROSOFT SOFTWARE LICENSE
  - MIT License

- 라이선스 위치
  - `PodoRenderer\packages\Microsoft.Direct3D.D3D12.<버전명>\LICENSE.txt`
  - `PodoRenderer\packages\Microsoft.Direct3D.D3D12.<버전명>\LICENSE-CODE.txt`

<br>




### 6.2. DirectX Tool Kit (DirectXTK12)
- 목적
  - Direct3D 12 유틸리티 라이브러리 사용

- 사용 방식
  - NuGet 패키지 관리 (directxtk12_desktop_win10)

- 복원 위치
  - `PodoRenderer\packages\directxtk12_desktop_win10.<버전명>`

- 라이선스 종류
  - MIT License

- 라이선스 위치
  - `PodoRenderer\packages\directxtk12_desktop_win10.<버전명>\docs\README.md` 내부 간접 링크
  - `https://github.com/microsoft/DirectXTK12/blob/main/LICENSE`

<br>




### 6.3. PIX Event Runtime
- 목적
  - PIX 캡처

- 사용 방식
  - NuGet 패키지 관리 (WinPixEventRuntime)
  
- 복원 위치
  - `PodoRenderer\packages\WinPixEventRuntime.<버전명>`

- 라이선스 종류
  - MIT License

- 라이선스 위치
  - `PodoRenderer\packages\WinPixEventRuntime.<버전명>\license.txt`

<br>




### 6.4 Dear ImGui
- 목적
  - GUI 제공

- 사용 방식
  - 리포지토리 내부에 직접 포함함(`PodoRenderer\External\imgui`)
  - 예제 코드의 일부를 수정하여 사용함(`PodoRenderer\Code\Header\Alloc.h`)

- 원본 리포지토리
  - ocornut/imgui  
    `https://github.com/ocornut/imgui`

- 라이선스 종류
  - MIT License

- 라이선스 위치
  - `PodoRenderer\External\imgui\LICENSE.txt`

<br>




<!---------------------------------------------------------------------------------------------------------------------------------------------->
## 7. 구현 참고
### 7.1. Win32 관련
"Get Started with Win32 and C++" (Microsoft Learn / 2026년 10월 열람)  
[https://learn.microsoft.com/en-us/windows/win32/learnwin32/learn-to-program-for-windows](https://learn.microsoft.com/en-us/windows/win32/learnwin32/learn-to-program-for-windows)

"Setting the default DPI awareness for a process" (Microsoft Learn / 2026년 10월 열람)  
[https://learn.microsoft.com/en-us/windows/win32/hidpi/setting-the-default-dpi-awareness-for-a-process](https://learn.microsoft.com/en-us/windows/win32/hidpi/setting-the-default-dpi-awareness-for-a-process)

<br>




### 7.2. DirectX 12 관련
"Introduction To 3D Game Programming With DirectX 12 Second Edition" (Frank D. Luna 지음 / Mercury Learning And Information 출판 / 2025년 발행) 

"Graphics APIs in Windows" (Microsoft Learn / 2026년 10월 열람)  
[https://learn.microsoft.com/en-us/windows/win32/direct3darticles/graphics-apis-in-windows-vista](https://learn.microsoft.com/en-us/windows/win32/direct3darticles/graphics-apis-in-windows-vista) 

"WDDM architecture" (Microsoft Learn / 2026년 10월 열람)  
[https://learn.microsoft.com/en-us/windows-hardware/drivers/display/windows-vista-and-later-display-driver-model-architecture](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/windows-vista-and-later-display-driver-model-architecture)

"Benefits of the WDDM" (Microsoft Learn / 2026년 10월 열람)  
[https://learn.microsoft.com/en-us/windows-hardware/drivers/display/benefits-of-the-windows-vista-and-later-display-driver-model](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/benefits-of-the-windows-vista-and-later-display-driver-model)

<br>




### 7.3. 초기화 관련
"ComPtr" (Microsoft / 2026년 10월 열람)  
[https://github.com/Microsoft/DirectXTK/wiki/ComPtr](https://github.com/Microsoft/DirectXTK/wiki/ComPtr)

"Programming DirectX with COM" (Microsoft Learn / 2026년 10월 열람)  
[https://learn.microsoft.com/en-us/windows/win32/prog-dx-with-com](https://learn.microsoft.com/en-us/windows/win32/prog-dx-with-com)

"Hardware Feature Levels" (Microsoft Learn / 2026년 10월 열람)  
[https://learn.microsoft.com/en-us/windows/win32/direct3d12/hardware-feature-levels](https://learn.microsoft.com/en-us/windows/win32/direct3d12/hardware-feature-levels)

"Hardware Tiers" (Microsoft Learn / 2026년 11월 열람)  
[https://learn.microsoft.com/en-us/windows/win32/direct3d12/hardware-support](https://learn.microsoft.com/en-us/windows/win32/direct3d12/hardware-support)

"Anatomy of Direct3D 12 Create Device" (Chuck Walbourn / 2026년 10월 열람)  
[https://walbourn.github.io/anatomy-of-direct3d-12-create-device/](https://walbourn.github.io/anatomy-of-direct3d-12-create-device/)

<br>




### 7.4. 제시 관련
"DXGI flip model" (Microsoft Learn / 2026년 10월 열람)  
[https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/dxgi-flip-model](https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/dxgi-flip-model)

"For best performance, use DXGI flip model" (Microsoft Learn / 2026년 10월 열람)  
[https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/for-best-performance--use-dxgi-flip-model](https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/for-best-performance--use-dxgi-flip-model)

"DirectX 12: Presentation Modes In Windows 10" (Microsoft DirectX 12 and Graphics Education / 2026년 10월 열람)  
[https://youtu.be/E3wTajGZOsA?si=todYm9imRjgSOXju](https://youtu.be/E3wTajGZOsA?si=todYm9imRjgSOXju)

"The Care and Feeding of Modern Swap Chains" (Chuck Walbourn / 2026년 10월 열람)  
[https://walbourn.github.io/care-and-feeding-of-modern-swapchains](https://walbourn.github.io/care-and-feeding-of-modern-swapchains)  
[https://walbourn.github.io/care-and-feeding-of-modern-swap-chains-2](https://walbourn.github.io/care-and-feeding-of-modern-swap-chains-2)  
[https://walbourn.github.io/care-and-feeding-of-modern-swap-chains-3](https://walbourn.github.io/care-and-feeding-of-modern-swap-chains-3)

<br>




### 7.5. VRR 옵션 관련
"Variable refresh rate displays" (Microsoft Learn / 2026년 10월 열람)  
[https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/variable-refresh-rate-displays](https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/variable-refresh-rate-displays)

<br>




### 7.6. HDR 옵션 관련
"Linear-Space Lighting (i.e. Gamma)" (Flimic Worlds / 2026년 10월 열람)  
[https://filmicworlds.com/blog/linear-space-lighting-i-e-gamma/](https://filmicworlds.com/blog/linear-space-lighting-i-e-gamma/)

"Gamma-correct rendering" (Molecular Musings / 2026년 10월 열람)  
[https://blog.molecular-matters.com/2011/11/21/gamma-correct-rendering/](https://blog.molecular-matters.com/2011/11/21/gamma-correct-rendering/)

"Converting data for the color space" (Microsoft Learn / 2026년 10월 열람)  
[https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/converting-data-color-space](https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/converting-data-color-space)

"Use DirectX with Advanced Color on high/standard dynamic range displays" (Microsoft Learn / 2026년 10월 열람)  
[https://learn.microsoft.com/en-us/windows/win32/direct3darticles/high-dynamic-range](https://learn.microsoft.com/en-us/windows/win32/direct3darticles/high-dynamic-range)

<br>




### 7.7. 빌드 관련
"Link an executable to a DLL" (Microsoft Learn / 2026년 10월 열람)  
[https://learn.microsoft.com/en-us/cpp/build/linking-an-executable-to-a-dll?view=msvc-170](https://learn.microsoft.com/en-us/cpp/build/linking-an-executable-to-a-dll?view=msvc-170)

"Getting Started with the Agility SDK" (Microsoft Dev Blogs / 2026년 10월 열람)  
[https://devblogs.microsoft.com/directx/gettingstarted-dx12agility](https://devblogs.microsoft.com/directx/gettingstarted-dx12agility)

<br>




### 7.8. 측정 관련
"PresentMon" (GameTechDev / 2026년 10월 열람)  
[https://github.com/GameTechDev/PresentMon](https://github.com/GameTechDev/PresentMon)

"WinPixEventRuntime" (Microsoft Dev Blogs / 2026년 10월 열람)  
[https://devblogs.microsoft.com/pix/winpixeventruntime](https://devblogs.microsoft.com/pix/winpixeventruntime)

"Programmatic Timing Captures now available" (Microsoft Dev Blogs / 2026년 10월 열람)  
[https://devblogs.microsoft.com/pix/programmatic-timing-captures-now-available](https://devblogs.microsoft.com/pix/programmatic-timing-captures-now-available)

<br>



