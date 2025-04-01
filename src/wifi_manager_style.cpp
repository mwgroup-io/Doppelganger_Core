#include "wifi_manager_style.h"

const char *WIFI_MANAGER_CUSTOM_CSS = R"(
<style>
  :root {
    --arrow-dark-bg: #0F1014;
    --arrow-dark-secondary: #16171C;
    --arrow-dark-tertiary: #1D1E24;
    --arrow-dark-border: #2A2B32;
    --arrow-dark-text: #E5E9F0;
    --arrow-dark-text-secondary: #939AAD;
    --arrow-dark-blue: #7AA2F7;
    --arrow-dark-blue-secondary: #3D59A1;
    --arrow-dark-green: #9ECE6A;
    --arrow-dark-red: #F7768E;
    --arrow-dark-orange: #FF6B00;
    --arrow-dark-purple: #BB9AF7;
  }
  
  body.invert {
    background-color: var(--arrow-dark-bg);
    color: var(--arrow-dark-text);
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Helvetica, Arial, sans-serif;
    margin: 0;
    padding: 0;
    line-height: 1.5;
  }

  .invert h1 {
    color: var(--arrow-dark-text);
    font-size: 1.25rem;
    margin: 0.3rem 0;
    text-align: center;
  }
  
  .invert .wrap {
    background-color: var(--arrow-dark-secondary);
    border: 1px solid var(--arrow-dark-border);
    border-radius: 8px;
    box-shadow: 0 4px 12px rgba(0, 0, 0, 0.25);
    margin: 10px auto;
    max-width: 600px;
    width: 92%;
    overflow: hidden;
  }
  
  .invert a {
    color: var(--arrow-dark-blue);
    text-decoration: none;
    transition: all 0.2s ease;
  }
  
  .invert a:hover {
    color: var(--arrow-dark-blue-secondary);
    text-decoration: underline;
  }
  
  .invert button {
    background: linear-gradient(to bottom, var(--arrow-dark-orange), #CC5500);
    color: white;
    border: 1px solid rgba(255, 107, 0, 0.2);
    border-radius: 6px;
    padding: 6px 12px;
    cursor: pointer;
    font-weight: 600;
    font-size: 14px;
    transition: all 0.2s ease;
    margin: 5px 0;
    width: 100%;
    box-shadow: 0 2px 4px rgba(0, 0, 0, 0.15);
  }
  
  .invert button:hover {
    background: linear-gradient(to bottom, #FF8533, var(--arrow-dark-orange));
    transform: translateY(-1px);
    box-shadow: 0 3px 6px rgba(0, 0, 0, 0.2);
  }
  
  .invert input, .invert select {
    background-color: var(--arrow-dark-bg);
    border: 1px solid var(--arrow-dark-border);
    color: var(--arrow-dark-text);
    border-radius: 6px;
    padding: 6px 10px;
    margin: 4px 0;
    font-size: 14px;
    width: 100%;
    box-sizing: border-box;
    transition: all 0.2s ease;
  }
  
  .invert input:focus, .invert select:focus {
    border-color: var(--arrow-dark-orange);
    outline: none;
    box-shadow: 0 0 0 2px rgba(255, 107, 0, 0.1);
  }
  
  .invert .msg {
    background-color: var(--arrow-dark-tertiary);
    border: 1px solid var(--arrow-dark-border);
    border-radius: 6px;
    padding: 8px;
    margin: 8px 0;
    font-size: 14px;
    line-height: 1.5;
  }
  
  .invert .q {
    color: var(--arrow-dark-text);
    font-weight: 500;
    margin: 5px 0;
  }
  
  .invert .sep {
    height: 1px;
    background-color: var(--arrow-dark-border);
    margin: 8px 0;
    border: none;
  }
  
  /* Network signal indicators */
  .q.l:before {
    background-color: var(--arrow-dark-red);
    border-radius: 50%;
    transition: all 0.2s ease;
  }
  
  .q.l.lm:before, .q.l.m:before {
    background-color: var(--arrow-dark-orange);
    border-radius: 50%;
  }
  
  .q.l.m.h:before, .q.l.m.h.vh:before {
    background-color: var(--arrow-dark-green);
    border-radius: 50%;
  }
  
  /* WiFi network styling */
  .invert .networks {
    margin-top: 5px;
    padding: 0 8px;
  }
  
  .invert .scan-result {
    margin-bottom: 3px;
    padding: 5px 10px;
    border-radius: 6px;
    border: 1px solid transparent;
    transition: all 0.2s ease;
    position: relative;
  }
  
  .invert .scan-result:hover {
    background-color: var(--arrow-dark-tertiary);
    border-color: var(--arrow-dark-border);
    transform: translateX(2px);
  }
  
  /* Form layout */
  .invert form {
    padding: 8px;
  }
  
  .invert form > div {
    margin-bottom: 8px;
  }
  
  .invert .c {
    clear: both;
    width: 100%;
  }
  
  /* Menu styling */
  .invert menu {
    display: flex;
    flex-direction: column;
    padding: 0 8px 8px;
    margin: 0;
    gap: 5px;
  }
  
  /* Connected status */
  .invert .connected-status {
    text-align: center;
    background-color: var(--arrow-dark-tertiary);
    padding: 8px;
    margin: 0;
    border-bottom: 1px solid var(--arrow-dark-border);
    font-size: 14px;
    color: var(--arrow-dark-text-secondary);
  }
  
  .invert .connected-status strong {
    color: var(--arrow-dark-blue);
    font-weight: 600;
  }
  
  /* Device info on info page */
  .invert table {
    width: 100%;
    border-collapse: collapse;
    margin: 12px 0;
  }
  
  .invert table td {
    padding: 8px;
    border-bottom: 1px solid var(--arrow-dark-border);
  }
  
  .invert table td:first-child {
    font-weight: 600;
    width: 40%;
  }
  
  /* Animations */
  @keyframes fadeIn {
    from { opacity: 0; transform: translateY(10px); }
    to { opacity: 1; transform: translateY(0); }
  }
  
  .invert .wrap {
    animation: fadeIn 0.3s ease forwards;
  }
  
  /* Handle different pages */
  #infomode, #wifimode, #wifinomode, #restartmode, #exitmode, #updatemode {
    padding: 8px;
  }
</style>
)";