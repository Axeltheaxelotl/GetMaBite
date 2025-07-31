const wrapper = document.getElementById("tiles");

let collums = 0;
rows = 0;
let currentColor = null;

const generateRandomColor = () => {
  const hue = Math.floor(Math.random() * 361); 
  const saturation = 100; 
  const lightness = Math.floor(Math.random() * (80 - 30) + 30);
  return `hsl(${hue}, ${saturation}%, ${lightness}%)`;
};

let count = -1;

const resetTiles = () => {
    const tiles = document.querySelectorAll('.tile');
    tiles.forEach(tile => {
      tile.style.transform = 'scale(1)'; 
    });
  };

const handleOnClick = (index) => {
    resetTiles();
    currentColor = generateRandomColor();
    anime({
    targets: ".tile",
    backgroundColor: currentColor,
    scale: [
        {value: 0.1, easing: 'easeOutSine', duration: 100},
        {value: 1.1, easing: 'easeOutSine', duration: 100},
    ],
    rotate: {
        value: '90',
        easing: 'easeInOutSine',
        duration: 200,
    },
    delay: anime.stagger(50, { grid: [collums, rows], from: index }),
    });
};
const createTile = (index) => {
  const tile = document.createElement("div");
  tile.classList.add("tile");
  if (currentColor) {
    tile.style.backgroundColor = currentColor; 
  }
  tile.onclick = (e) => handleOnClick(index);
  return tile;
};

const createTiles = (quantity) => {
  Array.from(Array(quantity)).map((tile, index) => {
    wrapper.appendChild(createTile(index));
  });
};

createTiles(collums * rows);

window.onload = () => {
    setTimeout(() => {
      const centerRowIndex = Math.floor(rows / 2);
      const centerCollumIndex = Math.floor(collums / 2);
      const centerIndex = centerRowIndex * collums + centerCollumIndex;
      handleOnClick(centerIndex);
    }, 1000); // 2-second delay
};
const createGrid = () => {
  wrapper.innerHTML = "";
  collums = Math.floor(document.body.clientWidth / 50);
  rows = Math.floor(document.body.clientHeight / 50);
  wrapper.style.setProperty("--collums", collums);
  wrapper.style.setProperty("--rows", rows);
  createTiles(collums * rows);
};
createGrid();
window.onresize = () => createGrid();
