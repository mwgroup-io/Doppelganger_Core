// Author: @tweathers-sec
// Copyright: @tweathers-sec and Mayweather Group LLC

let savedTheme = localStorage.getItem("theme") || "arrow-dark";

function setTheme(theme) {
  document.documentElement.setAttribute("data-theme", theme);
  localStorage.setItem("theme", theme);
  savedTheme = theme;

  const themeSelect = document.getElementById("theme-select");
  if (themeSelect) {
    themeSelect.value = theme;
  }

  const logoImages = document.querySelectorAll(".footer .theme-logo");
  if (logoImages.length > 0) {
    logoImages.forEach((img) => {
      const timestamp = new Date().getTime();
      if (
        theme === "light" ||
        theme === "classic" ||
        theme === "github" ||
        theme === "arrow"
      ) {
        img.src = `img/doppelganger_lm.png?t=${timestamp}`;
      } else {
        img.src = `img/doppelganger_dm.png?t=${timestamp}`;
      }
    });
  }
}

function changeTheme(theme) {
  setTheme(theme);
}

function initTheme() {
  setTheme(savedTheme);
}

document.documentElement.setAttribute("data-theme", savedTheme);

document.addEventListener("DOMContentLoaded", function () {
  const logoImages = document.querySelectorAll(".footer .theme-logo");
  if (logoImages.length > 0) {
    logoImages.forEach((img) => {
      if (
        savedTheme === "light" ||
        savedTheme === "classic" ||
        savedTheme === "github" ||
        savedTheme === "arrow"
      ) {
        img.src = "img/doppelganger_lm.png";
      } else {
        img.src = "img/doppelganger_dm.png";
      }
    });
  }

  initTheme();
});

window.addEventListener("load", function () {
  document.body.classList.add("theme-loaded");
});
