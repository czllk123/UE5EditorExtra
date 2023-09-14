#pragma once

#include "CoreMinimal.h"
#include "NiagaraCommon.h"
#include "NiagaraTypes.h"
#include "NiagaraDataSet.h"
#include "WaterFall.h"


class AWaterFall;



class FCustomNiagaraDataSetAccessor
{
public:
	bool Init(const FNiagaraDataSetCompiledData& CompiledData, FName InVariableName);

	float ReadFloat(const FNiagaraDataBuffer* DataBuffer, uint32 Instance, uint32 Component) const;
	float ReadHalf(const FNiagaraDataBuffer* DataBuffer, uint32 Instance, uint32 Component) const;
	int32 ReadInt(const FNiagaraDataBuffer* DataBuffer, uint32 Instance, uint32 Component) const;

	static bool ValidateDataBuffer(const FNiagaraDataSetCompiledData& CompiledData, const FNiagaraDataBuffer* DataBuffer, uint32 iInstance, TFunction<void(const FNiagaraVariable&, int32)> ErrorCallback);
	static bool ValidateDataBuffer(const FNiagaraDataSetCompiledData& CompiledData, const FNiagaraDataBuffer* DataBuffer, TFunction<void(const FNiagaraVariable&, uint32, int32)> ErrorCallback);
	

	bool GetParticleInfoFromDataBuffer(const FNiagaraDataSetCompiledData& CompiledData, const FNiagaraDataBuffer* DataBuffer, uint32 Instance, FName ParticleVar, int32& OutInt);
	bool GetParticleInfoFromDataBuffer(const FNiagaraDataSetCompiledData& CompiledData, const FNiagaraDataBuffer* DataBuffer, uint32 Instance, FName ParticleVar, float& OutFloat);


	FName GetName() const { return VariableName; }
	
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
