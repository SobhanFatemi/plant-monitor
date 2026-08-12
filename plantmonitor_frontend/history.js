const API = "/api/readings/";

async function loadHistory() {
  const res = await fetch(API);
  const response = await res.json();
  const data = response.results;
  const chronological = [...data].reverse();

  document.getElementById("count").textContent =
    `${data.length} reading${data.length === 1 ? "" : "s"} total`;

  renderChart(chronological);
  renderTable(data); // keep newest-first for the table
  lucide.createIcons();
}

function renderChart(readings) {
  const ctx = document.getElementById("historyChart");
  const labels = readings.map(r => new Date(r.recorded_at).toLocaleString());
  const moisture = readings.map(r => r.moisture);

  new Chart(ctx, {
    type: 'line',
    data: {
      labels,
      datasets: [{
        label: 'Moisture %',
        data: moisture,
        borderColor: '#4ade80',
        backgroundColor: 'rgba(74, 222, 128, 0.1)',
        borderWidth: 1.5,
        pointRadius: 0,
        pointHoverRadius: 4,
        pointHoverBackgroundColor: '#4ade80',
        tension: 0.2,
        fill: true
      }]
    },
    options: {
      interaction: { mode: 'index', intersect: false },
      plugins: {
        legend: { display: false },
        tooltip: {
          backgroundColor: '#222',
          titleColor: '#eee',
          bodyColor: '#4ade80',
          borderColor: '#444',
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
          grid: { color: '#333' },
          ticks: {
            color: '#999',
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
        x: { grid: { color: '#222' }, ticks: { color: '#666', maxTicksLimit: 12 } }
      }
    }
  });
}

function renderTable(readings) {
  const body = document.getElementById("historyBody");
  if (readings.length === 0) {
    body.innerHTML = `<tr><td colspan="5">No readings yet.</td></tr>`;
    return;
  }

  body.innerHTML = readings.map(r => `
    <tr>
      <td>${new Date(r.recorded_at).toLocaleString()}</td>
      <td>${r.moisture.toFixed(2)}%</td>
      <td>${r.raw_moisture}</td>
      <td>${r.temperature.toFixed(1)}</td>
      <td>${r.humidity.toFixed(1)}</td>
    </tr>
  `).join("");
}

loadHistory();
