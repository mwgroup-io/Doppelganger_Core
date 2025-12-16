// Author: @tweathers-sec
// Copyright: @tweathers-sec and Mayweather Group LLC

let capturedCards;
let isPaxtonMode = false;

fetch("reader_config.json")
  .then((r) => r.json())
  .then((cfg) => {
    isPaxtonMode = cfg && cfg.READER_TYPE === "PAXTON";
  })
  .catch(() => {
    isPaxtonMode = false;
  })
  .finally(() => {
    fetch("cards.csv")
      .then((response) => response.text())
      .then((csvData) => {
        capturedCards = parseCSV(csvData, isPaxtonMode);
        setHeaders(isPaxtonMode);
        buildTable(capturedCards);
        attachSortHandlers();
      })
      .catch((error) => console.error("Error fetching CSV:", error));
  });

function attachSortHandlers() {
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
}

function setHeaders(paxton) {
  const headerRow = document.querySelector(".content-table tr");
  if (!headerRow) return;
  if (paxton) {
    headerRow.innerHTML = `
      <th class="content-head" data-column="TYPE" data-order="desc">Bits</th>
      <th class="content-head" data-column="TOKEN" data-order="desc">Token</th>
      <th class="content-head" data-column="HEX" data-order="desc">HEX Value</th>
    `;
  } else {
    headerRow.innerHTML = `
      <th class="content-head" data-column="BL" data-order="desc">Bit Length</th>
      <th class="content-head" data-column="FC" data-order="desc">Facility Code</th>
      <th class="content-head" data-column="CN" data-order="desc">Card Number</th>
    `;
  }
}

function parseCSV(csvData, paxton) {
  const lines = csvData
    .split("\n")
    .map((line) => line.trim())
    .filter(Boolean);
  const data = [];
  let keypadCNs = [];

  lines.forEach((line) => {
    const parts = line.split(",");
    if (parts.length < 3) return;
    const left0 = parts[0] || "";
    const dataType = left0.includes(":") ? left0.split(":")[1].trim() : "";

    if (paxton) {
      // Handle Paxton keypad presses
      if (dataType === "PAXTON_KEYPAD") {
        const keyPress =
          parts[5] && parts[5].includes(":")
            ? parts[5].split(":")[1].trim()
            : "";
        keypadCNs.push(keyPress);
        return;
      }

      // Handle regular Paxton cards
      if (dataType === "PAXTON") {
        // If we have pending keypad data, flush it first
        if (keypadCNs.length > 0) {
          data.push({ TYPE: "PIN", TOKEN: keypadCNs.join(""), HEX: "N/A" });
          keypadCNs = [];
        }

      const TYPE =
        parts[2] && parts[2].includes(":")
          ? parts[2].split(":")[1].trim()
          : "75";
      const HEX =
          parts[3] && parts[3].includes(":")
            ? parts[3].split(":")[1].trim()
            : "";
      const TOKEN =
          parts[5] && parts[5].includes(":")
            ? parts[5].split(":")[1].trim()
            : "";
      data.push({ TYPE, TOKEN, HEX });
        return;
      }

      // Ignore non-Paxton entries in Paxton mode
      return;
    }

    // HID mode - ignore Paxton entries
    if (dataType === "PAXTON" || dataType === "PAXTON_KEYPAD") {
      return;
    }

    let BL = parts[2].split(":")[1]?.trim() ?? "";
    let FC = parts[4].split(":")[1]?.trim() ?? "";
    let CN = parts[5].split(":")[1]?.trim() ?? "";

    BL = isNaN(BL) ? BL : parseInt(BL);
    FC = isNaN(FC) ? FC : parseInt(FC);
    CN = isNaN(CN) ? CN : parseInt(CN);

    if (dataType === "KEYPAD") {
      // Only aggregate keypad rows in HID mode
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

  if (keypadCNs.length > 0) {
    if (paxton) {
      data.push({ TYPE: "PIN", TOKEN: keypadCNs.join(""), HEX: "N/A" });
    } else {
    data.push({ BL: "PIN", FC: "N/A", CN: keypadCNs.join("") });
    }
  }

  return data;
}

function buildTable(data) {
  const table = document.getElementById("cardTable");
  if (!table) return;
  if (isPaxtonMode) {
    table.innerHTML = data
      .map(
        (row) => `
    <tr>
      <td>${row.TYPE ?? ""}</td>
      <td>${row.TOKEN ?? ""}</td>
      <td>${row.HEX ?? ""}</td>
    </tr>`
      )
      .join("");
  } else {
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
}
