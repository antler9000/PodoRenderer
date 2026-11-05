#pragma once
#define NOMINMAX

//ImGui
#include "imgui.h"

//Win32
#include <windows.h>

//D3D12
#include <d3d12.h>

//NOTE: Dear ImGui의 Win32 + D3D12 예제에 사용된 할당자를 수정하여 사용함
struct ImGuiDescriptorHeapAllocator
{
    ID3D12DescriptorHeap*       pHeap       = nullptr;
    D3D12_DESCRIPTOR_HEAP_TYPE  HeapType    = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
    D3D12_CPU_DESCRIPTOR_HANDLE HeapStartCpu;
    D3D12_GPU_DESCRIPTOR_HANDLE HeapStartGpu;
    UINT                        HeapHandleIncrement;
    ImVector<int>               FreeIndices;

    void Create(ID3D12Device* pGivenDevice, ID3D12DescriptorHeap* pGivenHeap, UINT capacity)
    {
        IM_ASSERT(pHeap == nullptr && FreeIndices.empty());
        pHeap = pGivenHeap;
        D3D12_DESCRIPTOR_HEAP_DESC desc = pGivenHeap->GetDesc();
        HeapType = desc.Type;
        HeapStartCpu = pHeap->GetCPUDescriptorHandleForHeapStart();
        HeapStartGpu = pHeap->GetGPUDescriptorHandleForHeapStart();
        HeapHandleIncrement = pGivenDevice->GetDescriptorHandleIncrementSize(HeapType);
        FreeIndices.reserve(capacity);
        for (int n = desc.NumDescriptors; n > 0; n--)
            FreeIndices.push_back(n - 1);
    }

    void Destroy()
    {
        pHeap = nullptr;
        FreeIndices.clear();
    }

    void Alloc(D3D12_CPU_DESCRIPTOR_HANDLE* pOutCpuDescHandle, D3D12_GPU_DESCRIPTOR_HANDLE* pOutGpuDescHandle)
    {
        IM_ASSERT(FreeIndices.Size > 0);
        int idx = FreeIndices.back();
        FreeIndices.pop_back();
        pOutCpuDescHandle->ptr = HeapStartCpu.ptr + static_cast<SIZE_T>(idx * HeapHandleIncrement);
        pOutGpuDescHandle->ptr = HeapStartGpu.ptr + static_cast<SIZE_T>(idx * HeapHandleIncrement);
    }

    void Free(D3D12_CPU_DESCRIPTOR_HANDLE outCpuDescHandle, D3D12_GPU_DESCRIPTOR_HANDLE outGpuDescHandle)
    {
        int cpu_idx = (int)((outCpuDescHandle.ptr - HeapStartCpu.ptr) / HeapHandleIncrement);
        int gpu_idx = (int)((outGpuDescHandle.ptr - HeapStartGpu.ptr) / HeapHandleIncrement);
        IM_ASSERT(cpu_idx == gpu_idx);
        FreeIndices.push_back(cpu_idx);
    }
};