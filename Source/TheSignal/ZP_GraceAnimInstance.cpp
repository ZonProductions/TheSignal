// Copyright The Signal. All Rights Reserved.

#include "ZP_GraceAnimInstance.h"
#include "ZP_GraceGameplayComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void UZP_GraceAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	CachedCharacter = Cast<ACharacter>(TryGetPawnOwner());
	if (CachedCharacter)
	{
		CachedMovement = CachedCharacter->GetCharacterMovement();
	}
}

void UZP_GraceAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!CachedCharacter || !CachedMovement)
	{
		// Re-attempt cache (handles late possession)
		CachedCharacter = Cast<ACharacter>(TryGetPawnOwner());
		if (CachedCharacter)
		{
			CachedMovement = CachedCharacter->GetCharacterMovement();
		}
		if (!CachedCharacter || !CachedMovement) return;
	}

	// Ground speed
	const FVector Velocity = CachedMovement->Velocity;
	Speed = Velocity.Size2D();

	// Movement direction relative to actor facing (-180 to 180)
	if (Speed > 1.0f)
	{
		const FRotator ActorRot = CachedCharacter->GetActorRotation();
		const FRotator DeltaRot = (Velocity.ToOrientationRotator() - ActorRot).GetNormalized();
		Direction = DeltaRot.Yaw;
	}
	else
	{
		Direction = 0.0f;
	}

	// Air state
	bIsInAir = CachedMovement->IsFalling();

	// Crouch state
	bIsCrouching = CachedMovement->IsCrouching();

	// Moving threshold
	bShouldMove = Speed > 3.0f && !bIsInAir;

	// Sprint + Gait from GameplayComponent
	if (UZP_GraceGameplayComponent* GC = CachedCharacter->FindComponentByClass<UZP_GraceGameplayComponent>())
	{
		bIsSprinting = GC->bIsSprinting;
		Gait = GC->GASPGait;
	}
}
