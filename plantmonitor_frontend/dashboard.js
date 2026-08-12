const API = "/api/readings/?page_size=20";
const MOISTURE_THRESHOLD = 30;

async function loadData() {
  const res = await fetch(API);
  const response = await res.json();
  const data = response.results;

  if (data.length === 0) {
      return;
  }

  const recent = [...data].reverse();
  const latest = data[0];

  document.getElementById("current").innerHTML = `
    <div class="stat"><h2>${latest.moisture.toFixed(1)}%</h2><p>Moisture</p></div>
    <div class="stat"><h2>${latest.temperature.toFixed(1)}°C</h2><p>Temperature</p></div>
    <div class="stat"><h2>${latest.humidity.toFixed(1)}%</h2><p>Humidity</p></div>
  `;

  document.getElementById("alert").innerHTML =
    latest.moisture < MOISTURE_THRESHOLD
      ? `<p class="alert"><i data-lucide="triangle-alert"></i> Your plant needs water!</p>`
      : "";

  lucide.createIcons();

  renderChart(recent);
}

let chart;

function renderChart(readings) {
  const ctx = document.getElementById("chart");
  const labels = readings.map(r =>
    new Date(r.recorded_at).toLocaleTimeString()
  );
  const moisture = readings.map(r => r.moisture);

  if (!chart) {
    chart = new Chart(ctx, {
      type: "line",
      data: {
        labels,
        datasets: [{
          label: "Moisture %",
          data: moisture,
          borderColor: "#4ade80",
          backgroundColor: "rgba(74, 222, 128, 0.1)",
          borderWidth: 2,
          pointRadius: 0,
          pointHoverRadius: 5,
          pointHoverBackgroundColor: "#4ade80",
          tension: 0.3,
          fill: true
        }]
      },
      options: {
        responsive: true,
        maintainAspectRatio: false,
        animation: false,

        interaction: {
          mode: "index",
          intersect: false
        },

        plugins: {
          legend: { display: false },

          title: {
            display: true,
            text: "Soil Moisture (%)",
            color: "#ccc",
            font: {
              size: 14,
              weight: "normal"
            },
            align: "start",
            padding: { bottom: 12 }
          },

          tooltip: {
            backgroundColor: "#222",
            titleColor: "#eee",
            bodyColor: "#4ade80",
            borderColor: "#444",
            borderWidth: 1,
            displayColors: false,

            callbacks: {
              label: (context) => {
                const raw = readings[context.dataIndex].raw_moisture;
                return `${context.parsed.y.toFixed(2)}%  (raw: ${raw})`;
              }
            }
          }
        },

        scales: {
          y: {
            grid: { color: "#333" },

            ticks: {
              color: "#999",
              callback: (value) => `${value.toFixed(1)}%`
            },

            afterDataLimits: (scale) => {
              const min = Math.min(...moisture);
              const max = Math.max(...moisture);
              const pad = Math.max((max - min) * 0.2, 0.5);

              scale.min = Math.max(0, min - pad);
              scale.max = Math.min(100, max + pad);
            }
          },

          x: {
            grid: { color: "#222" },
            ticks: {
              color: "#666",
              maxTicksLimit: 8
            }
          }
        }
      }
    });

    return;
  }

  chart.data.labels = labels;
  chart.data.datasets[0].data = moisture;

  chart.update("none");
}

loadData();
setInterval(loadData, 30000);
