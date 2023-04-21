import React, { useRef, useEffect, useState } from 'react';

const AutoScrollWrapper = ({ children }) => {
  const bottomElementRef = useRef(null);
  const [isScrolledToBottom, setIsScrolledToBottom] = useState(true);

  const handleScroll = () => {
    const container = document.documentElement;
    const { scrollTop, scrollHeight, clientHeight } = container;
    const atBottom = scrollTop + clientHeight >= scrollHeight;

    setIsScrolledToBottom(atBottom);
  };

  useEffect(() => {
    const onResize = () => {
      if (isScrolledToBottom) {
        bottomElementRef.current.scrollIntoView({ behavior: 'smooth' });
      }
    };

    window.addEventListener('resize', onResize);
    return () => window.removeEventListener('resize', onResize);
  }, [isScrolledToBottom]);

  useEffect(() => {
    if (isScrolledToBottom) {
      bottomElementRef.current.scrollIntoView({ behavior: 'smooth' });
    }
  }, [children, isScrolledToBottom]);

  useEffect(() => {
    window.addEventListener('scroll', handleScroll);
    return () => window.removeEventListener('scroll', handleScroll);
  }, []);

  return (
    <div>
      {children}
      <div ref={bottomElementRef} />
    </div>
  );
};

export default AutoScrollWrapper;
