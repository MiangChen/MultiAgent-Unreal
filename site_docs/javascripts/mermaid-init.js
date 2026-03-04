function renderMermaid() {
  if (!window.mermaid) {
    return;
  }

  mermaid.initialize({
    startOnLoad: false,
    securityLevel: "loose",
    theme: "default",
  });

  document.querySelectorAll(".mermaid").forEach((el) => {
    el.removeAttribute("data-processed");
  });

  mermaid.run({
    querySelector: ".mermaid",
  });
}

if (window.document$ && typeof window.document$.subscribe === "function") {
  window.document$.subscribe(renderMermaid);
} else {
  window.addEventListener("load", renderMermaid);
}
