from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True, slots=True)
class PendingReview:
    review_type: str
    catalog_key: str
