document.addEventListener("DOMContentLoaded", () => {
  if (document.querySelector(".home-landing")) {
    document.body.classList.add("home-page");
  }

  const viewer = document.querySelector("[data-snapshot-viewer]");
  if (!viewer) {
    return;
  }

  const image = viewer.querySelector("[data-snapshot-image]");
  const title = viewer.querySelector("[data-snapshot-title]");
  const description = viewer.querySelector("[data-snapshot-description]");
  const items = Array.from(viewer.querySelectorAll(".home-snapshot-item"));

  const activate = (item) => {
    items.forEach((entry) => {
      const isActive = entry === item;
      entry.classList.toggle("is-active", isActive);
      entry.setAttribute("aria-selected", isActive ? "true" : "false");
    });

    image.src = item.dataset.snapshotSrc;
    image.alt = item.dataset.snapshotAlt || "";
    title.textContent = item.dataset.snapshotTitle || "";
    description.textContent = item.dataset.snapshotDescription || "";
  };

  items.forEach((item) => {
    item.addEventListener("click", () => activate(item));
  });
});
