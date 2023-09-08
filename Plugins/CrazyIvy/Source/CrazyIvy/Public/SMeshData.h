//Copyright  2022 Tav Shande.All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/Tuple.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Components/StaticMeshComponent.h"

#include "SMeshData.generated.h"

/**
 * 
 */


USTRUCT(BlueprintType)
struct FTriangle
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Default")
		FVector A= FVector(0,0,0);
	UPROPERTY(BlueprintReadWrite, Category = "Default")
		FVector B = FVector(0, 0, 0);
	UPROPERTY(BlueprintReadWrite, Category = "Default")
		FVector C = FVector(0, 0, 0);

	UPROPERTY(BlueprintReadWrite, Category = "Default")
		FVector Norm= FVector(0, 0, 0);

};




static TArray <FTriangle> GetTriangles(const UStaticMeshComponent* StaticMeshComponent)
{
	////// Store the triangles in an array.
	TArray<FTriangle> Triangles{};

	if (!StaticMeshComponent->GetStaticMesh())
		return Triangles;

	const FStaticMeshLODResources& LODResources{ StaticMeshComponent->GetStaticMesh()->GetRenderData()->LODResources[0] };

	// Where StaticMesh is your static mesh and LODIndex is the index of the LOD whose buffers you want. 
	// If performance is important, you may want to use LODs with lower triangle counts here.

	// The positions vertex buffer.
	const FPositionVertexBuffer& PositionVertexBuffer{ LODResources.VertexBuffers.PositionVertexBuffer };
	// The index buffer.
	const FRawStaticIndexBuffer& IndexBuffer{ LODResources.IndexBuffer };

	for (int32 Index{ 0 }; Index < IndexBuffer.GetNumIndices(); Index += 3)
	{
		const uint32 IndexA{ IndexBuffer.GetIndex(Index) };
		const uint32 IndexB{ IndexBuffer.GetIndex(Index + 1) };
		const uint32 IndexC{ IndexBuffer.GetIndex(Index + 2) };

		const FVector PositionA{ PositionVertexBuffer.VertexPosition(IndexA) };
		const FVector PositionB{ PositionVertexBuffer.VertexPosition(IndexB) };
		const FVector PositionC{ PositionVertexBuffer.VertexPosition(IndexC) };

		const FVector TangantA{ LODResources.VertexBuffers.StaticMeshVertexBuffer.VertexTangentX(IndexA) };
		const FVector TangantB{ LODResources.VertexBuffers.StaticMeshVertexBuffer.VertexTangentY(IndexB) };
		const FVector TangantC{ LODResources.VertexBuffers.StaticMeshVertexBuffer.VertexTangentZ(IndexC) };


		FTriangle ttriangle;
		FVector MeshLocation = StaticMeshComponent->GetOwner()->GetActorLocation();
		FVector MeshScale = StaticMeshComponent->GetOwner()->GetActorScale3D();
		FRotator MeshRotation = StaticMeshComponent->GetOwner()->GetActorRotation();

		/*	ttriangle.A = (PositionA) ;
				ttriangle.B = (PositionB );
				ttriangle.C = (PositionC );*/

		ttriangle.A = MeshRotation.RotateVector(PositionA) * MeshScale + MeshLocation;
		ttriangle.B = MeshRotation.RotateVector(PositionB) * MeshScale + MeshLocation;
		ttriangle.C = MeshRotation.RotateVector(PositionC) * MeshScale + MeshLocation;

		//UE_LOG(LogTemp, Log, TEXT("ttriangle.A %s"), *ttriangle.A.ToString())
		//UE_LOG(LogTemp, Log, TEXT("ttriangle.B %s"), *ttriangle.B.ToString())
		//UE_LOG(LogTemp, Log, TEXT("ttriangle.C %s"), *ttriangle.C.ToString())




		FVector tmp1 = ttriangle.A - ttriangle.C;

		FVector tmp2 = ttriangle.C - ttriangle.B;

		FVector norm = norm.CrossProduct(tmp1, tmp2);

		ttriangle.Norm = norm.GetSafeNormal();



		Triangles.Add(ttriangle);

	}


	return Triangles;
	//// Now you have an array of triangles.
	//
	//// Note: There is no need to normalize the data in such a way. This is just an example.
	//
	//// Note: CPU access must be enabled on the mesh resources for this to work.
	//
}
