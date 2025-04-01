// Single declaration of savedTheme
let savedTheme = localStorage.getItem("theme") || "arrow-dark";

function setTheme(theme) {
  document.documentElement.setAttribute("data-theme", theme);
  localStorage.setItem("theme", theme);
  savedTheme = theme; // Update the single variable

  const themeSelect = document.getElementById("theme-select");
  if (themeSelect) {
    themeSelect.value = theme;
  }

  // Update logo based on theme - use a more specific selector
  const logoImages = document.querySelectorAll(".footer .theme-logo");
  if (logoImages.length > 0) {
    logoImages.forEach((img) => {
      // Force the image to reload by adding a timestamp to the URL
      const timestamp = new Date().getTime();
      // Light themes use light mode logo
      if (
        theme === "light" ||
        theme === "classic" ||
        theme === "github" ||
        theme === "arrow"
      ) {
        img.src = `img/doppelganger_lm.png?t=${timestamp}`;
        console.log(`Theme changed to ${theme}: Setting light logo`);
      }
      // Dark themes use dark mode logo
      else {
        img.src = `img/doppelganger_dm.png?t=${timestamp}`;
        console.log(`Theme changed to ${theme}: Setting dark logo`);
      }
    });
  } else {
    console.log("No footer images found to update");
  }
}

function changeTheme(theme) {
  setTheme(theme);
}

function initTheme() {
  // Use the already defined savedTheme variable
  setTheme(savedTheme);
}

// Apply the theme immediately
document.documentElement.setAttribute("data-theme", savedTheme);

// Update logo as soon as DOM is loaded
document.addEventListener("DOMContentLoaded", function () {
  const logoImages = document.querySelectorAll(".footer .theme-logo");
  if (logoImages.length > 0) {
    logoImages.forEach((img) => {
      // Light themes use light mode logo
      if (
        savedTheme === "light" ||
        savedTheme === "classic" ||
        savedTheme === "github" ||
        savedTheme === "arrow"
      ) {
        img.src = "img/doppelganger_lm.png";
        console.log(`Initial theme ${savedTheme}: Setting light logo`);
      }
      // Dark themes use dark mode logo
      else {
        img.src = "img/doppelganger_dm.png";
        console.log(`Initial theme ${savedTheme}: Setting dark logo`);
      }
    });
  }

  // Also initialize the theme
  initTheme();
});

// Add theme-loaded class to body after all content is loaded
window.addEventListener("load", function () {
  document.body.classList.add("theme-loaded");
});
