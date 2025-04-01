// Author: @tweathers-sec
// Copyright: @tweathers-sec and Mayweather Group LLC

let capturedCards;

fetch("cards.csv")
  .then((response) => response.text())
  .then((csvData) => {
    capturedCards = parseCSV(csvData);
    buildTable(capturedCards);
  })
  .catch((error) => console.error("Error fetching CSV:", error));

document.querySelectorAll("th").forEach((th) => {
  th.addEventListener("click", function () {
    const column = this.getAttribute("data-column");
    let order = this.getAttribute("data-order") || "asc";
    order = order === "asc" ? "desc" : "asc";
    this.setAttribute("data-order", order);

    capturedCards.sort((a, b) =>
      order === "asc"
        ? a[column] > b[column]
          ? 1
          : -1
        : a[column] < b[column]
        ? 1
        : -1
    );

    buildTable(capturedCards);
  });
});

function parseCSV(csvData) {
  const lines = csvData
    .split("\n")
    .map((line) => line.trim())
    .filter(Boolean);
  const data = [];
  let keypadCNs = [];

  lines.forEach((line) => {
    const parts = line.split(",");
    const dataType = parts[0].split(":")[1].trim();
    let BL = parts[1].split(":")[1].trim();
    let FC = parts[3].split(":")[1].trim();
    let CN = parts[4].split(":")[1].trim();

    BL = isNaN(BL) ? BL : parseInt(BL);
    FC = isNaN(FC) ? FC : parseInt(FC);
    CN = isNaN(CN) ? CN : parseInt(CN);

    if (dataType === "KEYPAD") {
      keypadCNs.push(CN);
    } else {
      if (keypadCNs.length > 0) {
        data.push({ BL: "PIN", FC: "N/A", CN: keypadCNs.join("") });
        keypadCNs = [];
      }
      if (FC !== 0 || CN !== 0) {
        data.push({ BL, FC, CN });
      }
    }
  });

  // If the CSV ends with keypad data, add them as one row.
  if (keypadCNs.length > 0) {
    data.push({ BL: "PIN", FC: "N/A", CN: keypadCNs.join("") });
  }

  return data;
}

function buildTable(data) {
  const table = document.getElementById("cardTable");
  table.innerHTML = data
    .map(
      (row) => `
    <tr>
      <td>${row.BL}</td>
      <td>${row.FC}</td>
      <td>${row.CN}</td>
    </tr>`
    )
    .join("");
}
