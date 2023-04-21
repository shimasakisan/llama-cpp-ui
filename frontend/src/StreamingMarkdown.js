import React, { useState } from 'react';
import ReactMarkdown from 'react-markdown';

export default function StreamingMarkdown() {
  const [markdown, setMarkdown] = useState('');

  async function streamData() {
    const response = await fetch('http://localhost:8080/stream');
    const reader = response.body.getReader();
    while (true) {
      const { done, value } = await reader.read();
      if (done) {
        break;
      }
      const text = new TextDecoder().decode(value);
      setMarkdown(prevMarkdown => prevMarkdown + text);
    }
  }

  return (
    <>
      <button onClick={streamData}>Stream Data</button>
      <ReactMarkdown>{markdown}</ReactMarkdown>
    </>
  );
}