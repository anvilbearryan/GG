#include "GG.h"
#include "Game/Utility/GGFunctionLibrary.h"
#include "PaperFlipbookComponent.h"
#include "PaperFlipbook.h"
TArray<FOverlapResult> UGGFunctionLibrary::OverlapResults = TArray<FOverlapResult>();
void  UGGFunctionLibrary::BlendFlipbookToComponent(UPaperFlipbookComponent* InFlipbookComponent, UPaperFlipbook* ToFlipbook)
{
	if (ToFlipbook && InFlipbookComponent->GetFlipbook() != ToFlipbook)
	{
		// find current playbkpos
		float currentFlipbookLengthInv = InFlipbookComponent->GetFlipbookLength();
		if (currentFlipbookLengthInv > 0.f)
		{
			currentFlipbookLengthInv = 1 / currentFlipbookLengthInv;
		}
		float currentPlaybackPositionNormalized =
			InFlipbookComponent->GetPlaybackPosition() * currentFlipbookLengthInv;
		// conserve playback position
		InFlipbookComponent->SetFlipbook(ToFlipbook);
		InFlipbookComponent->SetNewTime(currentPlaybackPositionNormalized * ToFlipbook->GetTotalDuration());
		InFlipbookComponent->Play();
	}
}