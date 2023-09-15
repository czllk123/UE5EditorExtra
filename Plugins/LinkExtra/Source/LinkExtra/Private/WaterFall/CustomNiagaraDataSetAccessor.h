#pragma once

#include "CoreMinimal.h"
#include "NiagaraCommon.h"
#include "NiagaraTypes.h"
#include "NiagaraDataSet.h"







class FCustomNiagaraDataSetAccessor
{
public:
	bool Init(const FNiagaraDataSetCompiledData& CompiledData, FName InVariableName);

	float ReadFloat(const FNiagaraDataBuffer* DataBuffer, uint32 Instance, uint32 Component) const;
	float ReadHalf(const FNiagaraDataBuffer* DataBuffer, uint32 Instance, uint32 Component) const;
	int32 ReadInt(const FNiagaraDataBuffer* DataBuffer, uint32 Instance, uint32 Component) const;

	
	
	template<typename  T>
	bool GetParticleDataFromDataBuffer(const FNiagaraDataSetCompiledData& CompiledData, const FNiagaraDataBuffer* DataBuffer, FName ParticleVar,  uint32 Instance, T& OutValue)
	{
	if(Init(CompiledData, ParticleVar))
	{
		// 根据 T 的类型进行相应的处理
		if constexpr (std::is_same_v<T, int32>)
		{
			for(uint32 iComponent=0; iComponent < NumComponentsInt32; ++iComponent)
			{
				OutValue = ReadInt(DataBuffer, Instance, iComponent);
			}
		}
		else if constexpr (std::is_same_v<T, FVector>)
		{
			FVector TempVector;
			for(uint32 iComponent=0; iComponent < NumComponentsFloat; ++iComponent)
			{
				float ComponentValue = ReadFloat(DataBuffer, Instance, iComponent);
				if (iComponent == 0) TempVector.X = ComponentValue;
				else if (iComponent == 1) TempVector.Y = ComponentValue;
				else if (iComponent == 2) TempVector.Z = ComponentValue;
				
			}
			OutValue = TempVector;
		}


		// 可以继续添加其他数据类型的处理逻辑

		return true;
	}
	return false;
	}

	FName GetName() const { return VariableName; }

	
	static bool ValidateDataBuffer(const FNiagaraDataSetCompiledData& CompiledData, const FNiagaraDataBuffer* DataBuffer, uint32 iInstance, TFunction<void(const FNiagaraVariable&, int32)> ErrorCallback);
	static bool ValidateDataBuffer(const FNiagaraDataSetCompiledData& CompiledData, const FNiagaraDataBuffer* DataBuffer, TFunction<void(const FNiagaraVariable&, uint32, int32)> ErrorCallback);
	
private:
	FName					VariableName;
	FNiagaraTypeDefinition	NiagaraType;
	bool					bIsEnum = false;
	bool					bIsBool = false;
	uint32					NumComponentsInt32 = 0;
	uint32					NumComponentsFloat = 0;
	uint32					NumComponentsHalf = 0;
	uint32					ComponentIndexInt32 = INDEX_NONE;
	uint32					ComponentIndexFloat = INDEX_NONE;
	uint32					ComponentIndexHalf = INDEX_NONE;

	
};
