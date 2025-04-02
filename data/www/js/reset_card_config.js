// Author: @tweathers-sec
// Copyright: @tweathers-sec and Mayweather Group LLC

document.addEventListener("DOMContentLoaded", function () {
  var resetCardValue;

  fetch("reset_card.json")
    .then((response) => response.json())
    .then((jsonData) => {
      resetCardValue = parseJSON(jsonData);
      buildTable(resetCardValue);
    });

  function parseJSON(jsonData) {
    var data = [];

    if (Array.isArray(jsonData)) {
      for (var i = 0; i < jsonData.length; i++) {
        var card = jsonData[i];
        addCardData(data, card);
      }
    } else {
      addCardData(data, jsonData);
    }

    return data;
  }

  function addCardData(data, card) {
    var RBL = parseInt(card.RBL || card.bitLength);
    var RFC = parseInt(card.RFC || card.facilityCode);
    var RCN = parseInt(card.RCN || card.cardNumber);

    if (RFC !== 0 || RCN !== 0) {
      data.push({ RBL, RFC, RCN });
    }
  }

  function buildTable(data) {
    var table = document.getElementById("cardTable");
    table.innerHTML = "";
    for (var i = 0; i < data.length; i++) {
      var row = `<tr>
                  <td>${data[i].RBL}</td>
                  <td>${data[i].RFC}</td>
                  <td>${data[i].RCN}</td>
             </tr>`;
      table.innerHTML += row;
    }
  }
});
